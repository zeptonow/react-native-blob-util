import {NativeModules} from 'react-native'

const ReactNativeBlobUtil: ReactNativeBlobUtilNative = NativeModules.ReactNativeBlobUtil

function createMediafile(filedata: Object<{ path: string, parentFolder: string, mimeType: string }>, mediatype: string): Promise {
    if ((!'parentFolder' in filedata)) filedata['parentFolder'] = '';
    return ReactNativeBlobUtil.createMediaFile(filedata, mediatype)
}