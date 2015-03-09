/*
Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "msgpack.hpp"

#include "minko/Types.hpp"
#include "minko/data/Provider.hpp"
#include "minko/deserialize/TypeDeserializer.hpp"
#include "minko/file/AbstractParser.hpp"
#include "minko/file/AbstractSerializerParser.hpp"
#include "minko/file/AssetLibrary.hpp"
#include "minko/file/Dependency.hpp"
#include "minko/file/GeometryParser.hpp"
#include "minko/file/LinkedAsset.hpp"
#include "minko/file/MaterialParser.hpp"
#include "minko/file/Options.hpp"
#include "minko/file/TextureParser.hpp"
#include "minko/file/TextureWriter.hpp"
#include "minko/material/Material.hpp"
#include "minko/render/Texture.hpp"

using namespace minko;
using namespace minko::deserialize;
using namespace minko::file;

std::unordered_map<uint, std::function<void(
    unsigned short,
    AssetLibrary::Ptr,
    Options::Ptr,
    const std::string&,
    const std::vector<unsigned char>&,
	std::shared_ptr<Dependency>,
	short,
    std::list<std::shared_ptr<component::JobManager::Job>>&
)>> 
AbstractSerializerParser::_assetTypeToFunction;

void
AbstractSerializerParser::registerAssetFunction(uint assetTypeId, AssetDeserializeFunction f)
{
	_assetTypeToFunction[assetTypeId] = f;
}

AbstractSerializerParser::AbstractSerializerParser()
{
}


void
AbstractSerializerParser::parse(const std::string&					filename,
								const std::string&					resolvedFilename,
								std::shared_ptr<Options>			options,
								const std::vector<unsigned char>&	data,
								AssetLibraryPtr						assetLibrary)
{
    _filename = filename;
    _resolvedFilename = resolvedFilename;
}

void
AbstractSerializerParser::extractDependencies(AssetLibraryPtr						assetLibrary,
											  const std::vector<unsigned char>&		data,
											  short									dataOffset,
											  unsigned int							dependenciesSize,
											  std::shared_ptr<Options>				options,
											  std::string&							assetFilePath)
{
	msgpack::object							msgpackObject;
	msgpack::zone							mempool;
	SerializedAsset							serializedAsset;

	auto nbDependencies = readShort(data, dataOffset);

	unsigned int offset = dataOffset + 2;

	for (int index = 0; index < nbDependencies; ++index)
	{
		if (offset >(dataOffset + dependenciesSize))
        {
            _error->execute(shared_from_this(), Error("DependencyParsingError", "Error while parsing dependencies"));
            return;
        }

		auto assetSize = readUInt(data, offset);

		offset += 4;

		msgpack::unpack((char*)&data[offset], assetSize, NULL, &mempool, &msgpackObject);
		msgpackObject.convert(&serializedAsset);

		deserializeAsset(serializedAsset, assetLibrary, options, assetFilePath);

		offset += assetSize;
	}
}

static
bool
loadAssetData(const std::string&            resolvedFilename,
              Options::Ptr                  options,
              std::vector<unsigned char>&   out)
{
    auto assetLoader = Loader::create();
    auto assetLoaderOptions = options->clone();

    assetLoader->options(assetLoaderOptions);

    assetLoaderOptions
        ->loadAsynchronously(false)
        ->storeDataIfNotParsed(false);

    auto fileSuccessfullyLoaded = true;

    auto errorSlot = assetLoader->error()->connect([&](Loader::Ptr, const Error& error)
    {
        fileSuccessfullyLoaded = false;
    });

    auto completeSlot = assetLoader->complete()->connect([&](Loader::Ptr assetLoaderThis)
    {
        out = assetLoaderThis->files().at(resolvedFilename)->data();
    });

    assetLoader
        ->queue(resolvedFilename)
        ->load();

    return fileSuccessfullyLoaded;
}

void
AbstractSerializerParser::deserializeAsset(SerializedAsset&				asset,
											AssetLibraryPtr				assetLibrary,
											std::shared_ptr<Options>	options,
											std::string&				assetFilePath)
{
	std::vector<unsigned char>	data;
	std::string					assetCompletePath	= assetFilePath + "/";
	std::string					resolvedPath		= "";
	unsigned short				metaData			= (asset.a0 & 0xFFFF0000) >> 16;

	asset.a0 = asset.a0 & 0x000000FF;

    if (asset.a0 < 10)
    {
		assetCompletePath += asset.a2;
		resolvedPath = asset.a2;
    }

	data.assign(asset.a2.begin(), asset.a2.end());

	if ((asset.a0 == serialize::AssetType::GEOMETRY_ASSET || asset.a0 == serialize::AssetType::EMBED_GEOMETRY_ASSET) &&
		_dependency->geometryReferenceExist(asset.a1) == false) // geometry
	{
        if (asset.a0 == serialize::AssetType::GEOMETRY_ASSET &&
            !loadAssetData(assetCompletePath, options, data))
        {
            _error->execute(
                shared_from_this(),
                Error("MissingGeometryDependency", assetCompletePath)
            );

            return;
        }

        _geometryParser->_jobList.clear();
		_geometryParser->dependency(_dependency);

		if (asset.a0 == serialize::AssetType::EMBED_GEOMETRY_ASSET)
			resolvedPath = "geometry_" + std::to_string(asset.a1);

		_geometryParser->parse(resolvedPath, assetCompletePath, options, data, assetLibrary);
		_dependency->registerReference(asset.a1, assetLibrary->geometry(_geometryParser->_lastParsedAssetName));
		_jobList.splice(_jobList.end(), _geometryParser->_jobList);
	}
	else if ((asset.a0 == serialize::AssetType::MATERIAL_ASSET || asset.a0 == serialize::AssetType::EMBED_MATERIAL_ASSET) &&
		_dependency->materialReferenceExist(asset.a1) == false) // material
	{
        if (asset.a0 == serialize::AssetType::MATERIAL_ASSET &&
            !loadAssetData(assetCompletePath, options, data))
        {
            _error->execute(
                shared_from_this(),
                Error("MissingMaterialDependency", assetCompletePath)
            );

            return;
        }

		_materialParser->_jobList.clear();
		_materialParser->dependency(_dependency);

		if (asset.a0 == serialize::AssetType::EMBED_MATERIAL_ASSET)
			resolvedPath = "material_" + std::to_string(asset.a1);

		_materialParser->parse(resolvedPath, assetCompletePath, options, data, assetLibrary);
		_dependency->registerReference(asset.a1, assetLibrary->material(_materialParser->_lastParsedAssetName));
		_jobList.splice(_jobList.end(), _materialParser->_jobList);
	}
    else if ((asset.a0 == serialize::AssetType::EMBED_TEXTURE_ASSET ||
        asset.a0 == serialize::AssetType::TEXTURE_ASSET) &&
			(_dependency->textureReferenceExist(asset.a1) == false || _dependency->getTextureReference(asset.a1) == nullptr)) // texture
	{
        if (asset.a0 == serialize::AssetType::EMBED_TEXTURE_ASSET)
        {
            auto imageFormat = static_cast<serialize::ImageFormat>(metaData);

            auto extension = serialize::extensionFromImageFormat(imageFormat);

            resolvedPath = std::to_string(asset.a1) + "." + extension;
            assetCompletePath += resolvedPath;
        }
        else
        {
            if (!loadAssetData(assetCompletePath, options, data))
            {
                _error->execute(
                    shared_from_this(),
                    Error("MissingTextureDependency", assetCompletePath)
                );

                return;
            }
        }

		auto extension = resolvedPath.substr(resolvedPath.find_last_of(".") + 1);

		std::shared_ptr<file::AbstractParser> parser = assetLibrary->loader()->options()->getParser(extension);

        static auto nameId = 0;
        auto uniqueName = resolvedPath;

        while (assetLibrary->texture(uniqueName) != nullptr)
            uniqueName = "texture" + std::to_string(nameId++);

        parser->parse(uniqueName, assetCompletePath, options, data, assetLibrary);

        auto texture = assetLibrary->texture(uniqueName);

        if (options->disposeTextureAfterLoading())
            texture->disposeData();

        _dependency->registerReference(asset.a1, texture);
    }
    else if (asset.a0 == serialize::AssetType::EMBED_TEXTURE_PACK_ASSET &&
             (_dependency->textureReferenceExist(asset.a1) == false ||
             _dependency->getTextureReference(asset.a1) == nullptr))
    {
        resolvedPath = "texture_" + std::to_string(asset.a1);

        if (assetLibrary->texture(resolvedPath) == nullptr)
        {
            const auto hasTextureHeaderSize = (((metaData & 0xf000) >> 15) == 1 ? true : false);
            auto textureHeaderSize = static_cast<unsigned int>(metaData & 0x0fff);

            _textureParser->textureHeaderSize(textureHeaderSize);
            _textureParser->dataEmbed(true);

            _textureParser->parse(resolvedPath, assetCompletePath, options, data, assetLibrary);

        	auto texture = assetLibrary->texture(resolvedPath);

        	if (options->disposeTextureAfterLoading())
        	    texture->disposeData();
		}
		_dependency->registerReference(asset.a1, assetLibrary->texture(resolvedPath));
	}
    else if (asset.a0 == serialize::AssetType::TEXTURE_PACK_ASSET)
    {
        deserializeTexture(metaData, assetLibrary, options, assetCompletePath, data, _dependency, asset.a1, _jobList);
    }
	else if (asset.a0 == serialize::AssetType::EFFECT_ASSET && _dependency->effectReferenceExist(asset.a1) == false) // effect
	{
		assetLibrary->loader()->queue(assetCompletePath);
		_dependency->registerReference(asset.a1, assetLibrary->effect(assetCompletePath));
	}
    else if (asset.a0 == serialize::AssetType::LINKED_ASSET)
    {
        msgpack::type::tuple<int, int, std::string, std::vector<unsigned char>, int> linkedAssetdata;

        {
            msgpack::unpacked unpacked;
            msgpack::unpack(&unpacked, asset.a2.c_str(), asset.a2.size());

            unpacked.get().convert(&linkedAssetdata);
        }

        auto linkedAssetOffset = linkedAssetdata.a0;
        auto linkedAssetFilename = linkedAssetdata.a2;
        const auto linkedAssetLinkType = static_cast<LinkedAsset::LinkType>(linkedAssetdata.a4);

        if (linkedAssetLinkType == LinkedAsset::LinkType::Internal)
        {
            linkedAssetOffset += internalLinkedContentOffset();

            linkedAssetFilename = assetCompletePath + File::removePrefixPathFromFilename(_resolvedFilename);
        }

        auto linkedAsset = LinkedAsset::create()
            ->offset(linkedAssetOffset)
            ->length(linkedAssetdata.a1)
            ->filename(linkedAssetFilename)
            ->data(linkedAssetdata.a3)
            ->linkType(linkedAssetLinkType);

        _dependency->registerReference(asset.a1, linkedAsset);
    }
	else
	{
        if (_assetTypeToFunction.find(asset.a0) != _assetTypeToFunction.end())
        {
            _assetTypeToFunction[asset.a0](
                metaData,
                assetLibrary,
                options,
                assetCompletePath,
                data,
                _dependency,
                asset.a1,
                _jobList
            );
        }
	}

	data.clear();
	data.shrink_to_fit();

	asset.a2.clear();
	asset.a2.shrink_to_fit();
}

std::string
AbstractSerializerParser::extractFolderPath(const std::string& filepath)
{
	unsigned found = filepath.find_last_of("/\\");

	return filepath.substr(0, found);
}

bool
AbstractSerializerParser::readHeader(const std::string&					filename,
                                     const std::vector<unsigned char>&     data,
                                     int                                   extension)
{
	_magicNumber = readInt(data, 0);

	// File should start with 0x4D4B03 (MK3). Last byte reserved for extensions (Material, Geometry...)
    if (_magicNumber != MINKO_SCENE_MAGIC_NUMBER + (extension & 0xFF))
    {
        _error->execute(shared_from_this(), Error("InvalidFile", "Invalid scene file '" + filename + "': magic number mismatch"));
        return false;
    }
    
	_version.version = readInt(data, 4);

    _version.major = int(data[4]);
    _version.minor = readShort(data, 5);
    _version.patch = int(data[7]);

    if (_version.major != MINKO_SCENE_VERSION_MAJOR || 
        _version.minor > MINKO_SCENE_VERSION_MINOR || 
        (_version.minor <= MINKO_SCENE_VERSION_MINOR && _version.patch < MINKO_SCENE_VERSION_PATCH))
	{
		auto fileVersion = std::to_string(_version.major) + "." + std::to_string(_version.minor) + "." + std::to_string(_version.patch);
		auto sceneVersion = std::to_string(MINKO_SCENE_VERSION_MAJOR) + "." + std::to_string(MINKO_SCENE_VERSION_MINOR) + "." + std::to_string(MINKO_SCENE_VERSION_PATCH);

        auto message = "File " + filename + " doesn't match serializer version (file has v" + fileVersion + " while current version is v" + sceneVersion + ")";

        std::cerr << message << std::endl;
        
        _error->execute(shared_from_this(), Error("InvalidFile", message));
        return false;
	}

	// Versions with the same MAJOR value but different MINOR or PATCH value should be compatible
#if DEBUG
    if (_version.minor != MINKO_SCENE_VERSION_MINOR || _version.patch != MINKO_SCENE_VERSION_PATCH)
	{
		auto fileVersion = std::to_string(_version.major) + "." + std::to_string(_version.minor) + "." + std::to_string(_version.patch);
		auto sceneVersion = std::to_string(MINKO_SCENE_VERSION_MAJOR) + "." + std::to_string(MINKO_SCENE_VERSION_MINOR) + "." + std::to_string(MINKO_SCENE_VERSION_PATCH);

		std::cout << "Warning: file " + filename + " is v" + fileVersion + " while current version is v" + sceneVersion << std::endl;
	}
#endif

	_fileSize = readUInt(data, 8);

	_headerSize = readShort(data, 12);

	_dependenciesSize = readUInt(data, 14);
	_sceneDataSize = readUInt(data, 18);

    return true;
}

void
AbstractSerializerParser::deserializeTexture(unsigned short     metaData,
                                             AssetLibrary::Ptr  assetLibrary,
                                             Options::Ptr       options,
                                             const std::string& assetCompletePath,
                                             const std::vector<unsigned char>& data,
                                             DependencyPtr      dependency,
                                             short              assetId,
                                             std::list<JobPtr>& jobs)
{
    auto existingTexture = assetLibrary->texture(assetCompletePath);

    if (existingTexture != nullptr)
    {
        dependency->registerReference(assetId, existingTexture);

        return;
    }

    auto assetHeaderSize = MINKO_SCENE_HEADER_SIZE + 2 + 2;

    const auto hasTextureHeaderSize = (((metaData & 0xf000) >> 15) == 1 ? true : false);
    auto textureHeaderSize = static_cast<unsigned int>(metaData & 0x0fff);

    auto textureOptions = options->clone();

    auto textureExists = true;

    if (!hasTextureHeaderSize)
    {
        auto textureHeaderLoader = Loader::create();
        auto textureHeaderOptions = textureOptions->clone()
            ->loadAsynchronously(false)
            ->seekingOffset(0)
            ->seekedLength(assetHeaderSize)
            ->storeDataIfNotParsed(false);

        textureHeaderLoader->options(textureHeaderOptions);

        auto textureHeaderLoaderErrorSlot = textureHeaderLoader->error()->connect(
            [&](Loader::Ptr textureHeaderLoaderThis, const Error& error)
            {
                textureExists = false;

                this->error()->execute(shared_from_this(), Error("MissingTextureDependency", assetCompletePath));
            }
        );

        auto textureHeaderLoaderCompleteSlot = textureHeaderLoader->complete()->connect(
            [&](Loader::Ptr textureHeaderLoaderThis) -> void
            {
                const auto& headerData = textureHeaderLoaderThis->files().at(assetCompletePath)->data();

                const auto textureHeaderSizeOffset = assetHeaderSize - 2;

                std::stringstream headerDataStream(std::string(
                    headerData.begin() + textureHeaderSizeOffset,
                    headerData.begin() + textureHeaderSizeOffset + 2
                ));

                headerDataStream >> textureHeaderSize;
            }
        );

        textureHeaderLoader
            ->queue(assetCompletePath)
            ->load();
    }

    textureOptions
        ->loadAsynchronously(false)
        ->seekingOffset(0)
        ->seekedLength(assetHeaderSize + textureHeaderSize)
        ->parserFunction([&](const std::string& extension) -> AbstractParser::Ptr
    {
        if (extension != std::string("texture"))
            return nullptr;

        auto textureParser = TextureParser::create();

        textureParser->textureHeaderSize(textureHeaderSize);
        textureParser->dataEmbed(false);

        return textureParser;
    });

    auto textureLoader = Loader::create();
    textureLoader->options(textureOptions);

    auto texture = render::AbstractTexture::Ptr();

    auto textureLoaderErrorSlot = textureLoader->error()->connect(
        [&](Loader::Ptr textureLoaderThis, const Error& error)
        {
            textureExists = false;

            this->error()->execute(shared_from_this(), Error("MissingTextureDependency", assetCompletePath));
        }
    );

    auto textureLoaderCompleteSlot = textureLoader->complete()->connect([&](Loader::Ptr textureLoaderThis)
    {
        texture = assetLibrary->texture(assetCompletePath);
    });

    textureLoader
        ->queue(assetCompletePath)
        ->load();

    if (!textureExists)
        return;

    if (textureOptions->disposeTextureAfterLoading())
        texture->disposeData();

    dependency->registerReference(assetId, texture);
}
