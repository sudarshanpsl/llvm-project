//===-- main.c --------------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
int main (int argc, char const *argv[])
{
    struct point_tag {
        int x;
        int y;
        char padding[0];
    }; //% self.expect("frame variable pt.padding[0]", DATA_TYPES_DISPLAYED_CORRECTLY, substrs = ["pt.padding[0] = "]); self.expect("frame variable pt.padding[1]", DATA_TYPES_DISPLAYED_CORRECTLY, substrs = ["pt.padding[1] = "]); self.expect("expression -- (pt.padding[0])", DATA_TYPES_DISPLAYED_CORRECTLY, substrs = ["(char)", " = "]); self.expect("image lookup -t point_tag", DATA_TYPES_DISPLAYED_CORRECTLY, substrs = ['padding[]']) # Once rdar://problem/12566646 is fixed, this should display correctly

    struct rect_tag {
        struct point_tag bottom_left;
        struct point_tag top_right;
    };
    struct point_tag pt = { 2, 3, {} }; //% self.
    struct rect_tag rect = {{1, 2, {}}, {3, 4, {}}};
    return 0; //% self.expect("expression -- &pt == (struct point_tag*)0", substrs = ['false'])
}
