/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   IR_img_00_png;
    const int            IR_img_00_pngSize = 7445;

    extern const char*   IR_img_01_png;
    const int            IR_img_01_pngSize = 8648;

    extern const char*   NewsCycleRegular_ttf;
    const int            NewsCycleRegular_ttfSize = 178732;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 3;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
