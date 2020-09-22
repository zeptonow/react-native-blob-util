#include "pch.h"
#include "RNFetchBlob.h"

#include <windows.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Security.Cryptography.Core.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>

#include <filesystem>

using namespace winrt;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Security::Cryptography::Core;


// createFile
winrt::fire_and_forget RNFetchBlob::createFile(
	std::string path,
	std::string content,
	std::string encoding,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
try
{
	//if (encoding.compare("uri") == 0)
	//{
	//	//get contents from path
	//	co_return;
	//}

	bool isUTF8{ encoding.compare("utf8") == 0 };

	winrt::hstring contentToInsert{ winrt::to_hstring(content) };
	Streams::IBuffer buffer{ isUTF8 ?
		CryptographicBuffer::ConvertStringToBinary(contentToInsert, BinaryStringEncoding::Utf8) :
		CryptographicBuffer::DecodeFromBase64String(contentToInsert) };

	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);

	StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
	try
	{
		StorageFile file{ co_await folder.CreateFileAsync(fileName, CreationCollisionOption::FailIfExists) };
		Streams::IRandomAccessStream stream{ co_await file.OpenAsync(FileAccessMode::ReadWrite) };
		co_await stream.WriteAsync(buffer);
	}
	catch (...)
	{
		promise.Reject("EEXIST: File already exists."); // TODO: Include filepath
		co_return;
	}
	promise.Resolve(path);
}
catch (const hresult_error& ex)
{
	hresult result{ ex.code() };
	if (result == 0x80070002) // FileNotFoundException
	{
		promise.Reject("ENOENT: File does not exist and could not be created"); // TODO: Include filepath
	}
	else
	{
		promise.Reject("EUNSPECIFIED: Failed to write to file.");
	}
}

winrt::fire_and_forget RNFetchBlob::createFileASCII(
	std::string path,
	winrt::Microsoft::ReactNative::JSValueArray dataArray,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
{
	co_return;
}


// writeFile
winrt::fire_and_forget RNFetchBlob::writeFile(
	std::string path,
	std::string encoding,
	bool append,
	winrt::Microsoft::ReactNative::ReactPromise<int> promise) noexcept
{
	co_return;
}

winrt::fire_and_forget RNFetchBlob::writeFileArray(
	std::string path,
	winrt::Microsoft::ReactNative::JSValueArray dataArray,
	bool append,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
{
	co_return;
}


// writeStream



// readStream



// mkdir
winrt::fire_and_forget RNFetchBlob::mkdir(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
{
	co_return;
}


// readFile
winrt::fire_and_forget RNFetchBlob::readFile(
	std::string path,
	std::string encoding,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
{
	co_return;
}


// hash
winrt::fire_and_forget RNFetchBlob::hash(
	std::string path,
	std::string algorithm,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
{
	co_return;
}


// ls
winrt::fire_and_forget RNFetchBlob::ls(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueArray> promise) noexcept
{
	co_return;
}


// mv
winrt::fire_and_forget RNFetchBlob::mv(
	std::string src, // from
	std::string dest, // to
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
{
	co_return;
}


// cp
winrt::fire_and_forget RNFetchBlob::cp(
	std::string src, // from
	std::string dest, // to
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
{
	co_return;
}


// exists
winrt::fire_and_forget RNFetchBlob::exists(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
{
	co_return;
}


// isDir
winrt::fire_and_forget RNFetchBlob::isDir(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
{
	co_return;
}


// unlink
winrt::fire_and_forget RNFetchBlob::unlink(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<void> promise) noexcept
{
	co_return;
}


// lstat
winrt::fire_and_forget RNFetchBlob::lstat(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
{
	co_return;
}


// stat
winrt::fire_and_forget RNFetchBlob::stat(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
{
	co_return;
}

// asset
winrt::fire_and_forget RNFetchBlob::asset(
	std::string filename,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
{
	co_return;
}

// df
winrt::fire_and_forget RNFetchBlob::df(
	winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept
{
	co_return;
}

void RNFetchBlob::splitPath(const std::string& fullPath, winrt::hstring& directoryPath, winrt::hstring& fileName) noexcept
{
	std::filesystem::path path(fullPath);
	path.make_preferred();

	directoryPath = path.has_parent_path() ? winrt::to_hstring(path.parent_path().c_str()) : L"";
	fileName = path.has_filename() ? winrt::to_hstring(path.filename().c_str()) : L"";
}

void RNFetchBlob::splitPath(const winrt::hstring& fullPath, winrt::hstring& directoryPath, winrt::hstring& folderName) noexcept
{
	std::filesystem::path path(winrt::to_string(fullPath));
	path.make_preferred();

	directoryPath = path.has_parent_path() ? winrt::to_hstring(path.parent_path().c_str()) : L"";
	folderName = path.has_filename() ? winrt::to_hstring(path.filename().c_str()) : L"";
}
