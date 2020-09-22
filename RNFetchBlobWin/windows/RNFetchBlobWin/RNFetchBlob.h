#pragma once
#include "NativeModules.h"

REACT_MODULE(RNFetchBlob, L"RNFetchBlob");
struct RNFetchBlob
{

public:
	// createFile
	REACT_METHOD(createFile);
	winrt::fire_and_forget createFile(
		std::string path,
		std::string content,
		std::string encoding,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;

	REACT_METHOD(createFileASCII);
	winrt::fire_and_forget createFileASCII(
		std::string path,
		winrt::Microsoft::ReactNative::JSValueArray dataArray,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;

	
	// writeFile
	REACT_METHOD(writeFile);
	winrt::fire_and_forget writeFile(
		std::string path,
		std::string encoding,
		bool append,
		winrt::Microsoft::ReactNative::ReactPromise<int> promise) noexcept;

	REACT_METHOD(writeFileArray);
	winrt::fire_and_forget writeFileArray(
		std::string path,
		winrt::Microsoft::ReactNative::JSValueArray dataArray,
		bool append,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;


	// writeStream



	// readStream



	// mkdir
	REACT_METHOD(mkdir);
	winrt::fire_and_forget mkdir(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;


	// readFile
	REACT_METHOD(readFile);
	winrt::fire_and_forget readFile(
		std::string path,
		std::string encoding,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;


	// hash
	REACT_METHOD(hash);
	winrt::fire_and_forget hash(
		std::string path,
		std::string algorithm,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;

	
	// ls
	REACT_METHOD(ls);
	winrt::fire_and_forget ls(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueArray> promise) noexcept;


	// mv
	REACT_METHOD(mv);
	winrt::fire_and_forget mv(
		std::string src, // from
		std::string dest, // to
		winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;


	// cp
	REACT_METHOD(cp);
	winrt::fire_and_forget cp(
		std::string src, // from
		std::string dest, // to
		winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;


	// exists
	REACT_METHOD(exists);
	winrt::fire_and_forget exists(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;


	// isDir
	REACT_METHOD(isDir);
	winrt::fire_and_forget isDir(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<bool> promise) noexcept;


	// unlink
	REACT_METHOD(unlink);
	winrt::fire_and_forget unlink(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<void> promise) noexcept;


	// lstat
	REACT_METHOD(lstat);
	winrt::fire_and_forget lstat(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept;


	// stat
	REACT_METHOD(stat);
	winrt::fire_and_forget stat(
		std::string path,
		winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept;


	// asset
	REACT_METHOD(asset);
	winrt::fire_and_forget asset(
		std::string filename,
		winrt::Microsoft::ReactNative::ReactPromise<std::string> promise) noexcept;


	// df
	REACT_METHOD(df);
	winrt::fire_and_forget df(
		winrt::Microsoft::ReactNative::ReactPromise<winrt::Microsoft::ReactNative::JSValueObject> promise) noexcept;


// Helper methods
private:
	void splitPath(const std::string& fullPath,
		winrt::hstring& directoryPath,
		winrt::hstring& fileName) noexcept;

	void splitPath(const winrt::hstring& fullPath,
		winrt::hstring& directoryPath,
		winrt::hstring& folderName) noexcept;

};
