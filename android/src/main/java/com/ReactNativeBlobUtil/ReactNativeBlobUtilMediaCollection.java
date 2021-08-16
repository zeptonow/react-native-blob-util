package com.ReactNativeBlobUtil;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;

import com.ReactNativeBlobUtil.Utils.FileDescription;

import java.io.File;

public class ReactNativeBlobUtilMediaCollection {

    public enum MediaType {
        Audio,
        Image,
        Video,
        Download
    }

    private Uri getMediaUri(MediaType mt) {
        Uri res = null;
        if (mt == MediaType.Audio) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                res = MediaStore.Audio.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
            } else {
                res = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
            }
        } else if (mt == MediaType.Video) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                res = MediaStore.Video.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
            } else {
                res = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
            }
        } else if (mt == MediaType.Image) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                res = MediaStore.Images.Media.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
            } else {
                res = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
            }
        } else if (mt == MediaType.Download) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                res = MediaStore.Downloads.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY);
            }
        }

        return res;
    }

    private String getRelativePath(MediaType mt) {
        if (mt == MediaType.Audio) return Environment.DIRECTORY_MUSIC;
        if (mt == MediaType.Video) return Environment.DIRECTORY_MOVIES;
        if (mt == MediaType.Image) return Environment.DIRECTORY_PICTURES;
        if (mt == MediaType.Download) return Environment.DIRECTORY_DOWNLOADS;
        return Environment.DIRECTORY_DOWNLOADS;
    }

    public Uri createNewMediaFile(FileDescription file, MediaType mt) {
        // Add a specific media item.
        Context appCtx = ReactNativeBlobUtil.RCTContext.getApplicationContext();
        ContentResolver resolver = appCtx.getContentResolver();

        ContentValues fileDetails = new ContentValues();
        String relativePath = getRelativePath(mt);
        String mimeType = getRelativePath(mt);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            fileDetails.put(MediaStore.MediaColumns.DATE_ADDED, System.currentTimeMillis() / 1000);
            fileDetails.put(MediaStore.MediaColumns.DATE_MODIFIED, System.currentTimeMillis() / 1000);
            fileDetails.put(MediaStore.MediaColumns.MIME_TYPE, mimeType);
            fileDetails.put(MediaStore.MediaColumns.DISPLAY_NAME, file.name);
            fileDetails.put(MediaStore.MediaColumns.RELATIVE_PATH, relativePath + '/' + file.partentFolder);

            Uri mediauri = getMediaUri(mt);

            // Keeps a handle to the new song's URI in case we need to modify it later.
            return resolver.insert(mediauri, fileDetails);
        }
        else{
           File directory = Environment.getExternalStoragePublicDirectory(relativePath);
           if(directory.canWrite()){
               File f = new File(relativePath + '/' + file.getFullPath());
           }
        }

        return null;
    }

}
