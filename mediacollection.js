import {NativeModules} from 'react-native';
import type {ReactNativeBlobUtilNative} from "types";

const ReactNativeBlobUtil: ReactNativeBlobUtilNative = NativeModules.ReactNativeBlobUtil;

function createMediafile(filedata: Object<{ path: string, parentFolder: string, mimeType: string }>, mediatype: string): Promise {
    if ((!'parentFolder' in filedata)) filedata['parentFolder'] = '';
    return ReactNativeBlobUtil.createMediaFile(filedata, mediatype);
}

function writeToMediafile(uri:string, data:string){
    return ReactNativeBlobUtil.writeToMediaFile(uri, data);
}

export default {
    createMediafile,
    writeToMediafile
};