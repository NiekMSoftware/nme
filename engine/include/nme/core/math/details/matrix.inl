#pragma once

// matrix.inl -- everything indexes the flat, column-major buffer: element (r,c) == data[c*Rows + r].
// that buffer aliases the `col` vectors, so results built here read back correctly
// through operator()/operator[] too



// EOF