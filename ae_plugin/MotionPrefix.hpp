#pragma once
#include "MotionLayout.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <array>
#include <cmath>
#include <string>
#include <string_view>

namespace starfield::adapter::motion_prefix {
namespace layout=native_nodes::motion_layout;
using Bank=std::array<double,1+layout::path_prefix_chunks>;
inline bool decode(const Bank& bank,std::wstring& out) noexcept try {
    for(std::size_t i=0;i<bank.size();++i)
        if(!std::isfinite(bank[i]) || std::floor(bank[i])!=bank[i] || bank[i]<0 ||
            bank[i]>(i?16777215:layout::path_prefix_bytes))return false;
    std::string bytes;
    for(int i=0;i<int(bank[0]);++i){const auto v=(static_cast<unsigned>(bank[1+i/3])>>(8*(i%3)))&255u;
        if(!v)return false;bytes.push_back(static_cast<char>(v));}
    if(bytes.empty()){out.clear();return true;}
    const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),nullptr,0);
    if(!size)return false;std::wstring text(size,L'\0');
    if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),text.data(),size)!=size)return false;
    out=std::move(text);return true;
}catch(...){return false;}
inline bool encode(std::wstring_view text,Bank& out) noexcept try {
    if(text.size()>layout::path_prefix_bytes || text.find(L'\0')!=text.npos)return false;
    Bank bank{};if(text.empty()){out=bank;return true;}
    const int size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);
    if(size<=0 || size>layout::path_prefix_bytes)return false;
    std::string bytes(size,'\0');
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),bytes.data(),size,nullptr,nullptr)!=size)return false;
    bank[0]=size;for(int i=0;i<size;++i)bank[1+i/3]+=double(static_cast<unsigned char>(bytes[i])<<(8*(i%3)));
    out=bank;return true;
}catch(...){return false;}
}
