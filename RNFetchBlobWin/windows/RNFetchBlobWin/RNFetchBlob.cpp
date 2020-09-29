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
		//promise.Reject()
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
void RNFetchBlob::mkdir(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept
try
{
	std::filesystem::path dirPath(path);
	dirPath.make_preferred();

	// Consistent with Apple's createDirectoryAtPath method and result, but not with Android's
	if (std::filesystem::create_directories(dirPath) == false)
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "ENOENT", "ENOENT: no such file or directory, open " + path });
	}
	else
	{
		promise.Resolve(true);
	}
}
catch (const hresult_error& ex)
{
	// "Unexpected error while making directory."
	promise.Reject(winrt::to_string(ex.message()).c_str());
	promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "EUNSPECIFIED", "Error creating folder " + path + ", error: " + winrt::to_string(ex.message().c_str()) });
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
void RNFetchBlob::exists(
	std::string path,
	std::function<void(bool, bool)> callback) noexcept
{
	std::filesystem::path fsPath(path);
	bool doesExist{ std::filesystem::exists(fsPath) };
	bool isDirectory{ doesExist ? std::filesystem::is_directory(fsPath) : false};
	
	callback(doesExist, isDirectory);
}


// unlink
winrt::fire_and_forget RNFetchBlob::unlink(
	std::string path,
	std::function<void(std::string, bool)> callback) noexcept
try
{
	if (std::filesystem::is_directory(path))
	{
		std::filesystem::path path(path);
		path.make_preferred();
		StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(path.c_str())) };
		co_await folder.DeleteAsync();
	}
	else
	{
		winrt::hstring directoryPath, fileName;
		splitPath(path, directoryPath, fileName);
		StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
		auto target{ co_await folder.GetItemAsync(fileName) };
		co_await target.DeleteAsync();
	}
	callback(nullptr, true);
}
catch (const hresult_error& ex)
{
	callback(winrt::to_string(ex.message()), false);
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
	std::function<void(std::string, winrt::Microsoft::ReactNative::JSValueObject&)> callback) noexcept
try
{
	auto localFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
	auto properties{ co_await localFolder.Properties().RetrievePropertiesAsync({L"System.FreeSpace", L"System.Capacity"}) };

	winrt::Microsoft::ReactNative::JSValueObject result;
	result["freeSpace"] = unbox_value<uint64_t>(properties.Lookup(L"System.FreeSpace"));
	result["totalSpace"] = unbox_value<uint64_t>(properties.Lookup(L"System.Capacity"));
	callback(nullptr, result);
}
catch (...)
{
	winrt::Microsoft::ReactNative::JSValueObject emptyObject;
	callback("Failed to get storage usage.", emptyObject);
}

void RNFetchBlob::splitPath(const std::string& fullPath, winrt::hstring& directoryPath, winrt::hstring& fileName) noexcept
{
	std::filesystem::path path(fullPath);
	path.make_preferred();

	directoryPath = path.has_parent_path() ? winrt::to_hstring(path.parent_path().c_str()) : L"";
	fileName = path.has_filename() ? winrt::to_hstring(path.filename().c_str()) : L"";
}
