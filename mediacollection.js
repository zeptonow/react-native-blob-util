import {NativeModules} from 'react-native';
import type {ReactNativeBlobUtilNative} from "types";

const ReactNativeBlobUtil: ReactNativeBlobUtilNative = NativeModules.ReactNativeBlobUtil;

function createMediafile(filedata: Object<{ path: string, parentFolder: string, mimeType: string }>, mediatype: string): Promise {
    if ((!'parentFolder' in filedata)) filedata['parentFolder'] = '';
    return ReactNativeBlobUtil.createMediaFile(filedata, mediatype);
}

function writeToMediafile(uri: string, path: string) {
    return ReactNativeBlobUtil.writeToMediaFile(uri, path);
}

function copyToInternal(contenturi: string, destpath: string) {
    return ReactNativeBlobUtil.copyToInternal(contenturi, destpath);
}

function getBlob(contenturi: string, encoding: string) {
    return ReactNativeBlobUtil.getBlob(contenturi, encoding);
}

export default {
    createMediafile,
    writeToMediafile,
    copyToInternal,
    getBlob
};