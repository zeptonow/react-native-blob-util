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


void RNFetchBlob::Initialize(winrt::Microsoft::ReactNative::ReactContext const& reactContext) noexcept
{
	m_reactContext = reactContext;
}

//
// RNFS implementations
//
void RNFetchBlob::ConstantsViaConstantsProvider(winrt::Microsoft::ReactNative::ReactConstantProvider& constants) noexcept
{
	// RNFetchBlob.DocumentDir
	constants.Add(L"DocumentDir", to_string(ApplicationData::Current().LocalFolder().Path()));

	// RNFetchBlob.CacheDir
	constants.Add(L"CacheDir", to_string(ApplicationData::Current().LocalCacheFolder().Path()));

	// RNFetchBlob.PictureDir
	constants.Add(L"PictureDir", UserDataPaths::GetDefault().Pictures());

	// RNFetchBlob.MusicDir
	constants.Add(L"MusicDir", UserDataPaths::GetDefault().Music());

	// RNFetchBlob.MovieDir
	constants.Add(L"MusicDir", UserDataPaths::GetDefault().Videos());

	// RNFetchBlob.DownloadDirectoryPath - IMPLEMENT for convenience? (absent in iOS and deprecated in Android)
	constants.Add(L"DownloadDir", UserDataPaths::GetDefault().Downloads());

	// RNFetchBlob.MainBundleDir
	constants.Add(L"MainBundleDir", to_string(Package::Current().InstalledLocation().Path()));
}

// createFile
winrt::fire_and_forget RNFetchBlob::createFile(
	std::string path,
	std::wstring content,
	std::string encoding,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
try
{
	bool isUTF8{ encoding.compare("utf8") == 0 };
	bool shouldExit{ false };
	Streams::IBuffer buffer;
	if (encoding.compare("uri") == 0)
	{
		try
		{
			winrt::hstring srcDirectoryPath, srcFileName;
			splitPath(content, srcDirectoryPath, srcFileName);
			StorageFolder srcFolder{ co_await StorageFolder::GetFolderFromPathAsync(srcDirectoryPath) };
			StorageFile srcFile{ co_await srcFolder.GetFileAsync(srcFileName) };
			buffer = co_await FileIO::ReadBufferAsync(srcFile);
		}
		catch (...)
		{
			shouldExit = true;
		}
	}
	else if (encoding.compare("utf8") == 0)
	{
		buffer = CryptographicBuffer::ConvertStringToBinary(content, BinaryStringEncoding::Utf8);
	}
	else if (encoding.compare("base64") == 0)
	{
		buffer = CryptographicBuffer::DecodeFromBase64String(content);
	}
	else
	{
		promise.Reject("Invalid encoding");
		shouldExit = true;
	}

	if (!shouldExit)
	{

		winrt::hstring destDirectoryPath, destFileName;
		splitPath(path, destDirectoryPath, destFileName);

		auto folder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };

		try
		{
			auto file{ co_await folder.CreateFileAsync(destFileName, CreationCollisionOption::FailIfExists) };
			auto stream{ co_await file.OpenAsync(FileAccessMode::ReadWrite) };
			co_await stream.WriteAsync(buffer);
		}
		catch (...)
		{
			promise.Reject("EEXIST: File already exists."); // TODO: Include filepath
			shouldExit = true;
		}
	}
	if (!shouldExit)
	{
		promise.Resolve(path);
	}
	co_return;
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
	try
{
	std::vector<byte> data;
	data.reserve(dataArray.size());
	for (auto& var : dataArray)
	{
		data.push_back(var.AsInt8());
	}

	Streams::IBuffer buffer{ CryptographicBuffer::CreateFromByteArray(data) };

	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);

	bool shouldExit{ false };
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
		shouldExit = true;
	}
	if (shouldExit)
	{
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


// writeFile
winrt::fire_and_forget RNFetchBlob::writeFile(
	std::string path,
	std::string encoding,
	std::wstring data,
	bool append,
	winrt::Microsoft::ReactNative::ReactPromise<int> promise) noexcept
try
{
	Streams::IBuffer buffer;
	if (encoding.compare("utf8") == 0)
	{
		buffer = Cryptography::CryptographicBuffer::ConvertStringToBinary(data, BinaryStringEncoding::Utf8);
	}
	else if (encoding.compare("base64") == 0)
	{
		buffer = Cryptography::CryptographicBuffer::DecodeFromBase64String(data);
	}
	else if (encoding.compare("uri") == 0)
	{
		winrt::hstring srcDirectoryPath, srcFileName;
		splitPath(data, srcDirectoryPath, srcFileName);
		StorageFolder srcFolder{ co_await StorageFolder::GetFolderFromPathAsync(srcDirectoryPath) };
		StorageFile srcFile{ co_await srcFolder.GetFileAsync(srcFileName) };
		buffer = co_await FileIO::ReadBufferAsync(srcFile);
	}
	else
	{
		promise.Reject("Invalid encoding");
	}

	winrt::hstring destDirectoryPath, destFileName;
	splitPath(path, destDirectoryPath, destFileName);
	StorageFolder destFolder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };
	StorageFile destFile{ nullptr };
	if (append)
	{
		destFile = co_await destFolder.CreateFileAsync(destFileName, CreationCollisionOption::OpenIfExists);
	}
	else
	{
		destFile = co_await destFolder.CreateFileAsync(destFileName, CreationCollisionOption::ReplaceExisting);
	}
	Streams::IRandomAccessStream stream{ co_await destFile.OpenAsync(FileAccessMode::ReadWrite) };

	if (append)
	{
		stream.Seek(UINT64_MAX);
	}
	co_await stream.WriteAsync(buffer);
	promise.Resolve(buffer.Length());
}
catch (...)
{
	promise.Reject("Failed to write");
}

winrt::fire_and_forget RNFetchBlob::writeFileArray(
	std::string path,
	winrt::Microsoft::ReactNative::JSValueArray dataArray,
	bool append,
	winrt::Microsoft::ReactNative::ReactPromise<int> promise) noexcept
	try
{
	std::vector<byte> data;
	data.reserve(dataArray.size());
	for (auto& var : dataArray)
	{
		data.push_back(var.AsInt8());
	}
	Streams::IBuffer buffer{ CryptographicBuffer::CreateFromByteArray(data) };

	winrt::hstring destDirectoryPath, destFileName;
	splitPath(path, destDirectoryPath, destFileName);
	StorageFolder destFolder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };
	StorageFile destFile{ nullptr };
	if (append)
	{
		destFile = co_await destFolder.CreateFileAsync(destFileName, CreationCollisionOption::OpenIfExists);
	}
	else
	{
		destFile = co_await destFolder.CreateFileAsync(destFileName, CreationCollisionOption::ReplaceExisting);
	}
	Streams::IRandomAccessStream stream{ co_await destFile.OpenAsync(FileAccessMode::ReadWrite) };

	if (append)
	{
		stream.Seek(UINT64_MAX);
	}
	co_await stream.WriteAsync(buffer);
	promise.Resolve(buffer.Length());

	co_return;
}
catch (...)
{
	promise.Reject("Failed to write");
}


// writeStream
winrt::fire_and_forget RNFetchBlob::writeStream(
	std::string path,
	std::string encoding,
	bool append,
	std::function<void(std::string, std::string, std::string)> callback) noexcept
try
{
	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);
	StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
	StorageFile file{ co_await folder.CreateFileAsync(fileName, CreationCollisionOption::OpenIfExists) };
	std::string temp{encoding};
	Streams::IRandomAccessStream stream{ co_await file.OpenAsync(FileAccessMode::ReadWrite) };
	if (append)
	{
		stream.Seek(stream.Size());
	}
	EncodingOptions encodingOption;
	if (encoding.compare("utf8") == 0)
	{
		encodingOption = EncodingOptions::UTF8;
	}
	else if (encoding.compare("base64") == 0)
	{
		encodingOption = EncodingOptions::BASE64;
	}
	else if (encoding.compare("ascii") == 0)
	{
		encodingOption = EncodingOptions::ASCII;
	}
	else
	{
		co_return;
	}

	// Define the length, in bytes, of the buffer.
	uint32_t length = 128;
	// Generate random data and copy it to a buffer.
	IBuffer buffer = Cryptography::CryptographicBuffer::GenerateRandom(length);
	// Encode the buffer to a hexadecimal string (for display).
	std::string streamId = winrt::to_string(Cryptography::CryptographicBuffer::EncodeToHexString(buffer));

	RNFetchBlobStream streamInstance{ stream, encodingOption };
	m_streamMap.try_emplace(streamId, streamInstance);

	callback("", "", streamId);
}
catch (const hresult_error& ex)
{
	callback("EUNSPECIFIED", "Failed to create write stream at path '" + path + "'; " + winrt::to_string(ex.message().c_str()), "");
}


// writeChunk
void RNFetchBlob::writeChunk(
	std::string streamId,
	std::wstring data,
	std::function<void(std::string)> callback) noexcept
try
{
	auto stream{ m_streamMap.find(streamId)->second };
	Streams::IBuffer buffer;
	if (stream.encoding == EncodingOptions::UTF8)
	{
		buffer = Cryptography::CryptographicBuffer::ConvertStringToBinary(data, BinaryStringEncoding::Utf8);
	}
	else if (stream.encoding == EncodingOptions::BASE64)
	{
		buffer = Cryptography::CryptographicBuffer::DecodeFromBase64String(data);
	}
	else
	{
		callback("Invalid encoding type");
		return;
	}
	stream.streamInstance.WriteAsync(buffer).get(); //Calls it synchronously
	callback("");
}
catch (const hresult_error& ex)
{
	callback(winrt::to_string(ex.message().c_str()));
}

void RNFetchBlob::writeArrayChunk(
	std::string streamId,
	winrt::Microsoft::ReactNative::JSValueArray dataArray,
	std::function<void(std::string)> callback) noexcept
	try
{
	auto stream{ m_streamMap.find(streamId)->second };
	std::vector<byte> data;
	data.reserve(dataArray.size());
	for (auto& var : dataArray)
	{
		data.push_back(var.AsInt8());
	}
	Streams::IBuffer buffer{ CryptographicBuffer::CreateFromByteArray(data) };

	stream.streamInstance.WriteAsync(buffer).get(); // Calls it synchronously
	callback("");
}
catch (const hresult_error& ex)
{
	callback(winrt::to_string(ex.message().c_str()));
}

// readStream - no promises, callbacks, only event emission
winrt::fire_and_forget RNFetchBlob::readStream(
	std::string path,
	std::string encoding,
	uint32_t bufferSize,
	uint64_t tick,
	const std::string streamId) noexcept
	try
{
	EncodingOptions usedEncoding;
	if (encoding.compare("utf8"))
	{
		usedEncoding = EncodingOptions::UTF8;
	}
	else if (encoding.compare("base64"))
	{
		usedEncoding = EncodingOptions::BASE64;
	}
	else if (encoding.compare("ascii"))
	{
		usedEncoding = EncodingOptions::ASCII;
	}
	else
	{
		//Wrong encoding
		co_return;
	}

	uint32_t chunkSize = usedEncoding == EncodingOptions::BASE64 ? 4095 : 4096;
	if (bufferSize > 0)
	{
		chunkSize = bufferSize;
	}

	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);
	StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
	StorageFile file{ co_await folder.GetFileAsync(fileName) };

	Streams::IRandomAccessStream stream{ co_await file.OpenAsync(FileAccessMode::Read) };
	Buffer buffer{ chunkSize };
	const TimeSpan time{ tick };
	IAsyncAction timer;

	for (;;)
	{
		auto readBuffer = co_await stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::None);
		if (readBuffer.Length() == 0)
		{
			break;
		}
		if (usedEncoding == EncodingOptions::BASE64)
		{
			// TODO: Investigate returning wstrings as parameters
			winrt::hstring base64Content{ Cryptography::CryptographicBuffer::EncodeToBase64String(readBuffer) };
			m_reactContext.CallJSFunction(L"RCTDeviceEventEmitter", L"emit", [&streamId, &base64Content](React::IJSValueWriter const& argWriter) {
				WriteValue(argWriter, streamId);
				argWriter.WriteObjectBegin();
				React::WriteProperty(argWriter, "event", L"data");
				React::WriteProperty(argWriter, "detail", base64Content);
				argWriter.WriteObjectEnd();
				});
		}
		else
		{
			// TODO: Investigate returning wstrings as parameters
			std::string utf8Content{ winrt::to_string(Cryptography::CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, readBuffer)) };
			if (usedEncoding == EncodingOptions::ASCII)
			{

				//std::string asciiContent{ winrt::to_string(utf8Content) };
				std::string asciiContent{ utf8Content };
				//std::wstring asciiResult{ winrt::to_hstring(asciiContent) };
				// emit ascii content
				m_reactContext.CallJSFunction(L"RCTDeviceEventEmitter", L"emit", streamId,
					winrt::Microsoft::ReactNative::JSValueObject{
						{"data", asciiContent},
					});
			}
			else
			{
				//emit utf8 content
				m_reactContext.CallJSFunction(L"RCTDeviceEventEmitter", L"emit", streamId,
					winrt::Microsoft::ReactNative::JSValueObject{
						{"data", utf8Content},
					});
			}
		}
		// sleep
		if (tick > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(tick));
		}
	}
}
catch (const hresult_error& ex)
{
	m_reactContext.CallJSFunction(L"RCTDeviceEventEmitter", L"emit", L"DownloadBegin",
		winrt::Microsoft::ReactNative::JSValueObject{
			{streamId, winrt::to_string(ex.message()).c_str()},
		});
}


// mkdir - Implemented, not tested
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
	promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "EUNSPECIFIED", "Error creating folder " + path + ", error: " + winrt::to_string(ex.message().c_str()) });
}


// readFile - TODO: try returning wchar array and investigate different return types
winrt::fire_and_forget RNFetchBlob::readFile(
	std::string path,
	std::string encoding,
	winrt::Microsoft::ReactNative::ReactPromise<std::wstring> promise) noexcept
try
{
	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);

	StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
	StorageFile file{ co_await folder.GetFileAsync(fileName) };

	Streams::IBuffer buffer{ co_await FileIO::ReadBufferAsync(file) };
	if (encoding.compare("base64") == 0)
	{
		std::wstring base64Content{ Cryptography::CryptographicBuffer::EncodeToBase64String(buffer) };
		promise.Resolve(base64Content);
	}
	else
	{
		std::wstring utf8Content{ Cryptography::CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, buffer) };
		if (encoding.compare("ascii") == 0)
		{
			std::string asciiContent{ winrt::to_string(utf8Content) };
			std::wstring asciiResult{ winrt::to_hstring(asciiContent) };
			promise.Resolve(asciiResult);
			co_return;
		}
		promise.Resolve(utf8Content);
	}
}
catch (const hresult_error& ex)
{
	hresult result{ ex.code() };
	if (result == 0x80070002) // FileNotFoundException
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "ENOENT", "ENOENT: no such file or directory, open " + path });
	}
	else if (result == 0x80070005) // UnauthorizedAccessException
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "EISDIR", "EISDIR: illegal operation on a directory, read" });
	}
	else
	{
		// "Failed to read file."
		promise.Reject(winrt::to_string(ex.message()).c_str());
	}
}


// hash - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::hash(
	std::string path,
	std::string algorithm,
	winrt::Microsoft::ReactNative::ReactPromise<std::wstring> promise) noexcept
	try
{
	// Note: SHA224 is not part of winrt 
	if (algorithm.compare("sha224") == 0)
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "Error", "WinRT does not offer sha224 encryption." });
		co_return;
	}

	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);

	StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
	StorageFile file{ co_await folder.GetFileAsync(fileName) };

	auto search{ availableHashes.find(algorithm) };
	if (search == availableHashes.end())
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "Error", "Invalid hash algorithm " + algorithm });
		co_return;
	}

	CryptographyCore::HashAlgorithmProvider provider{ search->second() };
	Streams::IBuffer buffer{ co_await FileIO::ReadBufferAsync(file) };

	auto hashedBuffer{ provider.HashData(buffer) };
	std::wstring result{ Cryptography::CryptographicBuffer::EncodeToHexString(hashedBuffer) };

	promise.Resolve(result);
}
catch (const hresult_error& ex)
{
	hresult result{ ex.code() };
	if (result == 0x80070002) // FileNotFoundException
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "ENOENT", "ENOENT: no such file or directory, open " + path });
	}
	else if (result == 0x80070005) // UnauthorizedAccessException
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "EISDIR", "EISDIR: illegal operation on a directory, read" });
	}
	else
	{
		// "Failed to get checksum from file."
		promise.Reject(winrt::to_string(ex.message()).c_str());
	}
}


// ls - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::ls(
	std::string path,
	winrt::Microsoft::ReactNative::ReactPromise<std::vector<std::string>> promise) noexcept
	try
{
	winrt::hstring directoryPath, fileName;
	splitPath(path, directoryPath, fileName);

	StorageFolder targetDirectory{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };

	std::vector<std::string> results;
	auto items{ co_await targetDirectory.GetItemsAsync() };
	for (auto item : items)
	{
		results.push_back(to_string(item.Name()));
	}
	promise.Resolve(results);
}
catch (const hresult_error& ex)
{
	hresult result{ ex.code() };
	if (result == 0x80070002) // FileNotFoundException
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "ENOTDIR", "Not a directory '" + path + "'" });
	}
	else
	{
		promise.Reject(winrt::Microsoft::ReactNative::ReactError{ "EUNSPECIFIED", winrt::to_string(ex.message()).c_str() });
	}
}


// mv - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::mv(
	std::string src, // from
	std::string dest, // to
	std::function<void(std::string)> callback) noexcept
{
	try
	{
		winrt::hstring srcDirectoryPath, srcFileName;
		splitPath(src, srcDirectoryPath, srcFileName);

		winrt::hstring destDirectoryPath, destFileName;
		splitPath(dest, destDirectoryPath, destFileName);

		StorageFolder srcFolder{ co_await StorageFolder::GetFolderFromPathAsync(srcDirectoryPath) };
		StorageFolder destFolder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };
		StorageFile file{ co_await srcFolder.GetFileAsync(srcFileName) };

		co_await file.MoveAsync(destFolder, destFileName, NameCollisionOption::ReplaceExisting);
		callback("");
	}
	catch (const hresult_error& ex)
	{
		hresult result{ ex.code() };
		if (result == 0x80070002) // FileNotFoundException
		{
			callback("Source file not found.");
		}
		else
		{
			callback(winrt::to_string(ex.message()).c_str());
		}
	}
}


// cp - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::cp(
	std::string src, // from
	std::string dest, // to
	std::function<void(std::string)> callback) noexcept
	try
{
	winrt::hstring srcDirectoryPath, srcFileName;
	splitPath(src, srcDirectoryPath, srcFileName);

	winrt::hstring destDirectoryPath, destFileName;
	splitPath(dest, destDirectoryPath, destFileName);

	StorageFolder srcFolder{ co_await StorageFolder::GetFolderFromPathAsync(srcDirectoryPath) };
	StorageFolder destFolder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };
	StorageFile file{ co_await srcFolder.GetFileAsync(srcFileName) };

	co_await file.CopyAsync(destFolder, destFileName, NameCollisionOption::FailIfExists);

	callback("");
}
catch (const hresult_error& ex)
{
	hresult result{ ex.code() };
	if (result == 0x80070002) // FileNotFoundException
	{
		callback("Source file not found.");
	}
	callback(winrt::to_string(ex.message()).c_str());
}


// exists - Implemented, not tested
void RNFetchBlob::exists(
	std::string path,
	std::function<void(bool, bool)> callback) noexcept
{
	std::filesystem::path fsPath(path);
	bool doesExist{ std::filesystem::exists(fsPath) };
	bool isDirectory{ std::filesystem::is_directory(fsPath) };

	callback(doesExist, isDirectory);
}


// unlink - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::unlink(
	std::string path,
	std::function<void(std::string, bool)> callback) noexcept
	try
{
	if (std::filesystem::is_directory(path))
	{
		std::filesystem::path unlinkPath(path);
		unlinkPath.make_preferred();
		StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(unlinkPath.c_str())) };
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
	callback("", true);
}
catch (const hresult_error& ex)
{
	callback(winrt::to_string(ex.message()), false);
}


// lstat - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::lstat(
	std::string path,
	std::function<void(std::string, winrt::Microsoft::ReactNative::JSValueArray&)> callback) noexcept
	try
{
	std::filesystem::path directory(path);
	directory.make_preferred();
	StorageFolder targetDirectory{ co_await StorageFolder::GetFolderFromPathAsync(directory.c_str()) };

	winrt::Microsoft::ReactNative::JSValueArray resultsArray;

	auto items{ co_await targetDirectory.GetItemsAsync() };
	for (auto item : items)
	{
		auto properties{ co_await item.GetBasicPropertiesAsync() };

		winrt::Microsoft::ReactNative::JSValueObject itemInfo;

		itemInfo["filename"] = to_string(item.Name());
		itemInfo["path"] = to_string(item.Path());
		itemInfo["size"] = properties.Size();
		itemInfo["type"] = item.IsOfType(StorageItemTypes::Folder) ? "directory" : "file";
		itemInfo["lastModified"] = properties.DateModified().time_since_epoch() / std::chrono::seconds(1) - UNIX_EPOCH_IN_WINRT_SECONDS;

		resultsArray.push_back(std::move(itemInfo));
	}

	callback("", resultsArray);
}
catch (const hresult_error& ex)
{
	// "Failed to read directory."
	winrt::Microsoft::ReactNative::JSValueArray emptyArray;
	callback("failed to lstat path `" + path + "` because it does not exist or it is not a folder", emptyArray);
}


// stat - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::stat(
	std::string path,
	std::function<void(std::string, winrt::Microsoft::ReactNative::JSValueObject&)> callback) noexcept
	try
{
	std::filesystem::path givenPath(path);
	givenPath.make_preferred();

	std::string resultPath{ winrt::to_string(givenPath.c_str()) };
	auto potentialPath{ winrt::to_hstring(resultPath) };
	bool isFile{ false };
	uint64_t mtime{ 0 };
	uint64_t size{ 0 };

	// Try to open as folder
	try
	{
		StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(potentialPath) };
		auto properties{ co_await folder.GetBasicPropertiesAsync() };
		mtime = properties.DateModified().time_since_epoch() / std::chrono::seconds(1) - UNIX_EPOCH_IN_WINRT_SECONDS;
		size = properties.Size();
	}
	catch (...)
	{
		isFile = true;
		auto lastCharIndex{ path.length() - 1 };
		if (resultPath[lastCharIndex] == '\\')
		{
			givenPath = std::filesystem::path(resultPath.substr(0, lastCharIndex));
		}
	}

	// Try to open as file
	if (isFile)
	{
		auto directoryPath{ givenPath.has_parent_path() ? winrt::to_hstring(givenPath.parent_path().c_str()) : L"" };
		auto fileName{ givenPath.has_filename() ? winrt::to_hstring(givenPath.filename().c_str()) : L"" };

		StorageFolder folder{ co_await StorageFolder::GetFolderFromPathAsync(directoryPath) };
		auto properties{ (co_await folder.GetFileAsync(fileName)).Properties() };
		auto propertyMap{ co_await properties.RetrievePropertiesAsync({ L"System.DateCreated", L"System.DateModified", L"System.Size" }) };
		mtime = winrt::unbox_value<DateTime>(propertyMap.Lookup(L"System.DateModified")).time_since_epoch() / std::chrono::seconds(1) - UNIX_EPOCH_IN_WINRT_SECONDS;
		size = winrt::unbox_value<uint64_t>(propertyMap.Lookup(L"System.Size"));
	}

	winrt::Microsoft::ReactNative::JSValueObject fileInfo;
	fileInfo["size"] = size;
	fileInfo["filename"] = givenPath.filename().string();
	fileInfo["path"] = givenPath.string();
	fileInfo["lastModified"] = mtime;
	fileInfo["type"] = isFile ? "file" : "directory";

	callback("", fileInfo);
}
catch (const hresult_error& ex)
{
	winrt::Microsoft::ReactNative::JSValueObject emptyObject;
	callback(winrt::to_string(ex.message()).c_str(), emptyObject);
}


// df - Implemented, not tested
winrt::fire_and_forget RNFetchBlob::df(
	std::function<void(std::string, winrt::Microsoft::ReactNative::JSValueObject&)> callback) noexcept
	try
{
	auto localFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
	auto properties{ co_await localFolder.Properties().RetrievePropertiesAsync({L"System.FreeSpace", L"System.Capacity"}) };

	winrt::Microsoft::ReactNative::JSValueObject result;
	result["free"] = unbox_value<uint64_t>(properties.Lookup(L"System.FreeSpace"));
	result["total"] = unbox_value<uint64_t>(properties.Lookup(L"System.Capacity"));
	callback("", result);
}
catch (...)
{
	winrt::Microsoft::ReactNative::JSValueObject emptyObject;
	callback("Failed to get storage usage.", emptyObject);
}


winrt::fire_and_forget RNFetchBlob::slice(
	std::string src,
	std::string dest,
	uint32_t start,
	uint32_t end,
	winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept
	try
{
	winrt::hstring srcDirectoryPath, srcFileName, destDirectoryPath, destFileName;
	splitPath(src, srcDirectoryPath, srcFileName);
	splitPath(src, destDirectoryPath, destFileName);

	StorageFolder srcFolder{ co_await StorageFolder::GetFolderFromPathAsync(srcDirectoryPath) };
	StorageFolder destFolder{ co_await StorageFolder::GetFolderFromPathAsync(destDirectoryPath) };

	StorageFile srcFile{ co_await srcFolder.GetFileAsync(srcFileName) };
	StorageFile destFile{ co_await destFolder.CreateFileAsync(destFileName, CreationCollisionOption::OpenIfExists) };

	uint32_t length{ end - start };
	Streams::IBuffer buffer;
	Streams::IRandomAccessStream stream{ co_await srcFile.OpenAsync(FileAccessMode::Read) };
	stream.Seek(start);
	stream.ReadAsync(buffer, length, Streams::InputStreamOptions::None);
	co_await FileIO::WriteBufferAsync(destFile, buffer);

	promise.Resolve(dest);
}
catch (...)
{
	promise.Reject("Unable to slice file");
}


void RNFetchBlob::fetchBlob(
	winrt::Microsoft::ReactNative::JSValueObject options,
	std::string taskId,
	std::string method,
	std::string url,
	winrt::Microsoft::ReactNative::JSValueObject headers,
	std::string body,
	std::function<void(std::string, std::string, std::string)> callback) noexcept
{

}

void RNFetchBlob::fetchBlobForm(
	winrt::Microsoft::ReactNative::JSValueObject options,
	std::string taskId,
	std::string method,
	std::string url,
	winrt::Microsoft::ReactNative::JSValueObject headers,
	winrt::Microsoft::ReactNative::JSValueArray body,
	std::function<void(std::string)> callback) noexcept
{

}

void RNFetchBlob::enableProgressReport(
	std::string taskId,
	int interval,
	int count) noexcept
{
}

// enableUploadProgressReport
void RNFetchBlob::enableUploadProgressReport(
	std::string taskId,
	int interval,
	int count) noexcept
{
}

// cancelRequest
void RNFetchBlob::cancelRequest(
	std::string taskId,
	std::function<void(std::string)> callback) noexcept
{
}

void RNFetchBlob::removeSession(
	winrt::Microsoft::ReactNative::JSValueObject paths,
	std::function<void(std::string)> callback) noexcept
{

}

void RNFetchBlob::closeStream(
	std::string streamId,
	std::function<void(std::string)> callback) noexcept
try
{
	auto stream{ m_streamMap.find(streamId)->second };
	stream.streamInstance.Close();
	m_streamMap.extract(streamId);
}
catch (...)
{

}


void RNFetchBlob::splitPath(const std::string& fullPath, winrt::hstring& directoryPath, winrt::hstring& fileName) noexcept
{
	std::filesystem::path path{ fullPath };
	path.make_preferred();

	directoryPath = path.has_parent_path() ? winrt::to_hstring(path.parent_path().c_str()) : L"";
	fileName = path.has_filename() ? winrt::to_hstring(path.filename().c_str()) : L"";
}

void RNFetchBlob::splitPath(const std::wstring& fullPath, winrt::hstring& directoryPath, winrt::hstring& fileName) noexcept
{
	std::filesystem::path path{ fullPath };
	path.make_preferred();

	directoryPath = path.has_parent_path() ? winrt::to_hstring(path.parent_path().c_str()) : L"";
	fileName = path.has_filename() ? winrt::to_hstring(path.filename().c_str()) : L"";
}



RNFetchBlobStream::RNFetchBlobStream(Streams::IRandomAccessStream& _streamInstance, EncodingOptions _encoding) noexcept
	: streamInstance{ std::move(_streamInstance) }
	, encoding{ _encoding }
{
}

