#pragma once
#ifndef _TOOLS_HPP_
#define _TOOLS_HPP_

namespace engine {
namespace tools {
template <typename SizeT>
inline void hash_combine_impl(SizeT &seed, SizeT value) {
  seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
} // namespace tools
} // namespace engine

#endif //_TOOLS_HPP_
