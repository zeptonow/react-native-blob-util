#pragma once
#include "NativeModules.h"

REACT_MODULE(RNFetchBlobConst, L"RNFetchBlobConst");
struct RNFetchBlobConst
{
public:
    const std::string RNFB_RESPONSE_BASE64 = "base64";
    const std::string RNFB_RESPONSE_UTF8 = "utf8";
    const std::string RNFB_RESPONSE_PATH = "path";
};