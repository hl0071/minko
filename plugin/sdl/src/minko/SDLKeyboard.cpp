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

#include "minko/SDLKeyboard.hpp"

#if defined(EMSCRIPTEN)
# include "minko/MinkoWebGL.hpp"
# include "SDL/SDL.h"
# include "emscripten/emscripten.h"
#elif defined(MINKO_ANGLE)
# include "SDL2/SDL.h"
# include "SDL2/SDL_syswm.h"
# include <EGL/egl.h>
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
#else
# include "SDL2/SDL.h"
#endif

#if defined(__APPLE__)
# include <TargetConditionals.h>
# if TARGET_OS_IPHONE
#  include "SDL2/SDL_opengles.h"
# endif
#endif

using namespace minko;

const std::array<int, 256> SCAN_CODE_SDL_MAP
{
    {
        0,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
        12,
        13,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        32,
        33,
        34,
        35,
        36,
        37,
        38,
        39,
        40,
        41,
        42,
        43,
        44,
        45,
        46,
        47,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        58,
        59,
        60,
        61,
        62,
        63,
        64,
        65,
        66,
        67,
        68,
        69,
        70,
        71,
        72,
        73,
        74,
        75,
        76,
        77,
        78,
        79,
        80,
        81,
        82,
        83,
        84,
        85,
        86,
        87,
        88,
        89,
        90,
        91,
        92,
        93,
        94,
        95,
        96,
        97,
        98,
        99,
        100,
        101,
        102,
        103,
        104,
        105,
        106,
        107,
        108,
        109,
        110,
        111,
        112,
        113,
        114,
        115,
        116,
        117,
        118,
        119,
        120,
        121,
        122,
        123,
        124,
        125,
        126,
        127,
        128,
        129,
        133,
        134,
        135,
        136,
        137,
        138,
        139,
        140,
        141,
        142,
        143,
        144,
        145,
        146,
        147,
        148,
        149,
        150,
        151,
        152,
        153,
        154,
        155,
        156,
        157,
        158,
        159,
        160,
        161,
        162,
        163,
        164,
        176,
        177,
        178,
        179,
        180,
        181,
        182,
        183,
        184,
        185,
        186,
        187,
        188,
        189,
        190,
        191,
        192,
        193,
        194,
        195,
        196,
        197,
        198,
        199,
        200,
        201,
        202,
        203,
        204,
        205,
        206,
        207,
        208,
        209,
        210,
        211,
        212,
        213,
        214,
        215,
        216,
        217,
        218,
        219,
        220,
        221,
        224,
        225,
        226,
        227,
        228,
        229,
        230,
        231,
        257,
        258,
        259,
        260,
        261,
        262,
        263,
        264,
        265,
        266,
        267,
        268,
        269,
        270,
        271,
        272,
        273,
        274,
        275,
        276,
        277,
        278,
        279,
        280,
        281,
        282,
        283,
        284
    }
};

const std::array<int, 256> KEY_CODE_SDL_MAP
{
    {
        0,
        0,
        8,
        9,
        12,
        13,
        19,
        27,
        32,
        33,
        34,
        35,
        36,
        38,
        39,
        40,
        41,
        42,
        43,
        44,
        45,
        46,
        47,
        48,
        49,
        50,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        58,
        59,
        60,
        61,
        62,
        63,
        64,
        91,
        92,
        93,
        94,
        95,
        96,
        97,
        98,
        99,
        100,
        101,
        102,
        103,
        104,
        105,
        106,
        107,
        108,
        109,
        110,
        111,
        112,
        113,
        114,
        115,
        116,
        117,
        118,
        119,
        120,
        121,
        122,
        127,
        160,
        161,
        162,
        163,
        164,
        165,
        166,
        167,
        168,
        169,
        170,
        171,
        172,
        173,
        174,
        175,
        176,
        177,
        178,
        179,
        180,
        181,
        182,
        183,
        184,
        185,
        186,
        187,
        188,
        189,
        190,
        191,
        192,
        193,
        194,
        195,
        196,
        197,
        198,
        199,
        200,
        201,
        202,
        203,
        204,
        205,
        206,
        207,
        208,
        209,
        210,
        211,
        212,
        213,
        214,
        215,
        216,
        217,
        218,
        219,
        220,
        221,
        222,
        223,
        224,
        225,
        226,
        227,
        228,
        229,
        230,
        231,
        232,
        233,
        234,
        235,
        236,
        237,
        238,
        239,
        240,
        241,
        242,
        243,
        244,
        245,
        246,
        247,
        248,
        249,
        250,
        251,
        252,
        253,
        254,
        255,
        256,
        257,
        258,
        259,
        260,
        261,
        262,
        263,
        264,
        265,
        266,
        267,
        268,
        269,
        270,
        271,
        272,
        273,
        274,
        275,
        276,
        277,
        278,
        279,
        280,
        281,
        282,
        283,
        284,
        285,
        286,
        287,
        288,
        289,
        290,
        291,
        292,
        293,
        294,
        295,
        296,
        300,
        301,
        302,
        303,
        304,
        305,
        306,
        307,
        308,
        309,
        310,
        311,
        312,
        313,
        314,
        315,
        316,
        317,
        318,
        319,
        320,
        321,
        322
    }
};


const std::map<input::Keyboard::ScanCode, input::Keyboard::KeyCode> ScanCodeKeyCodeMap =
{
    //{ input::Keyboard::ScanCode::A, input::Keyboard::KeyCode::a },
    //{ input::Keyboard::ScanCode::E, input::Keyboard::KeyCode::e }
};

const std::map<input::Keyboard::KeyCode, input::Keyboard::ScanCode> KeyCodeScanCodeMap =
{
    //{ input::Keyboard::KeyCode::a, input::Keyboard::ScanCode::A },
    //{ input::Keyboard::KeyCode::e, input::Keyboard::ScanCode::E }
};

SDLKeyboard::SDLKeyboard()
{
    _keyboardState = SDL_GetKeyboardState(NULL);

    /*
    int keyCode1 = static_cast<int>(SDL_GetScancodeFromKey(static_cast<int>(input::Keyboard::KeyCode::a)));
    int keyCode2 = static_cast<int>(KeyCodeScanCodeMap.find(input::Keyboard::KeyCode::a)->second);

    std::cout << "A: \n" << "Key code: " << keyCode1
        << "\nKey code: " << keyCode2
        << std::endl;
    */

    std::cout << static_cast<int>(SDL_SCANCODE_TO_KEYCODE(static_cast<int>(input::Keyboard::ScanCode::B))) << std::endl;
}

bool SDLKeyboard::keyIsDown(input::Keyboard::ScanCode scanCode)
{
#if defined(EMSCRIPTEN)
    return _keyboardState[static_cast<int>(getKeyCodeFromScanCode(scanCode))] != 0;
#else
    return _keyboardState[static_cast<int>(scanCode)] != 0;
#endif
}

bool SDLKeyboard::keyIsDown(input::Keyboard::KeyCode keyCode)
{
#if defined(EMSCRIPTEN)
    // Note: bug in emscripten, GetKeyStates is indexed by key codes.
    return _keyboardState[static_cast<int>(keyCode)] != 0;
#else
    return _keyboardState[static_cast<int>(getScanCodeFromKeyCode(keyCode))] != 0;
#endif
}

input::Keyboard::KeyCode SDLKeyboard::getKeyCodeFromScanCode(input::Keyboard::ScanCode scanCode)
{
    auto iterator = ScanCodeKeyCodeMap.find(scanCode);

    if (iterator != ScanCodeKeyCodeMap.end())
        return iterator->second;
    else
        return static_cast<input::Keyboard::KeyCode>(SDL_SCANCODE_TO_KEYCODE(static_cast<int>(scanCode)));

}

input::Keyboard::ScanCode SDLKeyboard::getScanCodeFromKeyCode(input::Keyboard::KeyCode keyCode)
{
    auto iterator = KeyCodeScanCodeMap.find(keyCode);

    if (iterator != KeyCodeScanCodeMap.end())
        return iterator->second;
    else
        return static_cast<input::Keyboard::ScanCode>(SDL_GetScancodeFromKey(static_cast<int>(keyCode)));
}
