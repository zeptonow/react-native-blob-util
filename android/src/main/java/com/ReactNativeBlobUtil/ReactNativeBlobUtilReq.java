package com.ReactNativeBlobUtil;

import android.app.DownloadManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.util.Base64;
import android.webkit.CookieManager;

import androidx.annotation.NonNull;

import com.ReactNativeBlobUtil.Response.ReactNativeBlobUtilDefaultResp;
import com.ReactNativeBlobUtil.Response.ReactNativeBlobUtilFileResp;
import com.ReactNativeBlobUtil.Utils.Tls12SocketFactory;
import com.facebook.common.logging.FLog;
import com.facebook.react.bridge.Arguments;
import com.facebook.react.bridge.Callback;
import com.facebook.react.bridge.ReadableArray;
import com.facebook.react.bridge.ReadableMap;
import com.facebook.react.bridge.ReadableMapKeySetIterator;
import com.facebook.react.bridge.WritableArray;
import com.facebook.react.bridge.WritableMap;
import com.facebook.react.modules.core.DeviceEventManagerModule;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.Proxy;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.util.ArrayList;
import java.util.HashMap;

import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import javax.net.ssl.SSLContext;

import okhttp3.Call;
import okhttp3.ConnectionPool;
import okhttp3.ConnectionSpec;
import okhttp3.Headers;
import okhttp3.Interceptor;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;
import okhttp3.TlsVersion;


public class ReactNativeBlobUtilReq extends BroadcastReceiver implements Runnable {

    enum RequestType {
        Form,
        SingleFile,
        AsIs,
        WithoutBody,
        Others
    }

    enum ResponseType {
        KeepInMemory,
        FileStorage
    }

    enum ResponseFormat {
        Auto,
        UTF8,
        BASE64
    }

    public static HashMap<String, Call> taskTable = new HashMap<>();
    public static HashMap<String, Long> androidDownloadManagerTaskTable = new HashMap<>();
    static HashMap<String, ReactNativeBlobUtilProgressConfig> progressReport = new HashMap<>();
    static HashMap<String, ReactNativeBlobUtilProgressConfig> uploadProgressReport = new HashMap<>();
    static ConnectionPool pool = new ConnectionPool();

    ReactNativeBlobUtilConfig options;
    String taskId;
    String method;
    String url;
    String rawRequestBody;
    String destPath;
    ReadableArray rawRequestBodyArray;
    ReadableMap headers;
    Callback callback;
    long contentLength;
    long downloadManagerId;
    ReactNativeBlobUtilBody requestBody;
    RequestType requestType;
    ResponseType responseType;
    ResponseFormat responseFormat = ResponseFormat.Auto;
    WritableMap respInfo;
    boolean timeout = false;
    ArrayList<String> redirects = new ArrayList<>();
    OkHttpClient client;

    public ReactNativeBlobUtilReq(ReadableMap options, String taskId, String method, String url, ReadableMap headers, String body, ReadableArray arrayBody, OkHttpClient client, final Callback callback) {
        this.method = method.toUpperCase(Locale.ROOT);
        this.options = new ReactNativeBlobUtilConfig(options);
        this.taskId = taskId;
        this.url = url;
        this.headers = headers;
        this.callback = callback;
        this.rawRequestBody = body;
        this.rawRequestBodyArray = arrayBody;
        this.client = client;

        if (this.options.fileCache || this.options.path != null)
            responseType = ResponseType.FileStorage;
        else
            responseType = ResponseType.KeepInMemory;


        if (body != null)
            requestType = RequestType.SingleFile;
        else if (arrayBody != null)
            requestType = RequestType.Form;
        else
            requestType = RequestType.WithoutBody;
    }

    public static void cancelTask(String taskId) {
        if (taskTable.containsKey(taskId)) {
            Call call = taskTable.get(taskId);
            call.cancel();
            taskTable.remove(taskId);
        }

        if (androidDownloadManagerTaskTable.containsKey(taskId)) {
            long downloadManagerIdForTaskId = androidDownloadManagerTaskTable.get(taskId).longValue();
            Context appCtx = ReactNativeBlobUtil.RCTContext.getApplicationContext();
            DownloadManager dm = (DownloadManager) appCtx.getSystemService(Context.DOWNLOAD_SERVICE);
            dm.remove(downloadManagerIdForTaskId);
        }
    }

    @Override
    public void run() {

        // use download manager instead of default HTTP implementation
        if (options.addAndroidDownloads != null && options.addAndroidDownloads.hasKey("useDownloadManager")) {

            if (options.addAndroidDownloads.getBoolean("useDownloadManager")) {
                Uri uri = Uri.parse(url);
                DownloadManager.Request req = new DownloadManager.Request(uri);
                if (options.addAndroidDownloads.hasKey("notification") && options.addAndroidDownloads.getBoolean("notification")) {
                    req.setNotificationVisibility(DownloadManager.Request.VISIBILITY_VISIBLE_NOTIFY_COMPLETED);
                } else {
                    req.setNotificationVisibility(DownloadManager.Request.VISIBILITY_HIDDEN);
                }
                if (options.addAndroidDownloads.hasKey("title")) {
                    req.setTitle(options.addAndroidDownloads.getString("title"));
                }
                if (options.addAndroidDownloads.hasKey("description")) {
                    req.setDescription(options.addAndroidDownloads.getString("description"));
                }
                if (options.addAndroidDownloads.hasKey("path")) {
                    req.setDestinationUri(Uri.parse("file://" + options.addAndroidDownloads.getString("path")));
                }
                // #391 Add MIME type to the request
                if (options.addAndroidDownloads.hasKey("mime")) {
                    req.setMimeType(options.addAndroidDownloads.getString("mime"));
                }
                if (options.addAndroidDownloads.hasKey("mediaScannable") && options.addAndroidDownloads.getBoolean("mediaScannable")) {
                    req.allowScanningByMediaScanner();
                }
                // set headers
                ReadableMapKeySetIterator it = headers.keySetIterator();
                while (it.hasNextKey()) {
                    String key = it.nextKey();
                    req.addRequestHeader(key, headers.getString(key));
                }
                // Attempt to add cookie, if it exists
                URL urlObj;
                try {
                    urlObj = new URL(url);
                    String baseUrl = urlObj.getProtocol() + "://" + urlObj.getHost();
                    String cookie = CookieManager.getInstance().getCookie(baseUrl);
                    req.addRequestHeader("Cookie", cookie);
                } catch (MalformedURLException e) {
                    e.printStackTrace();
                }
                Context appCtx = ReactNativeBlobUtil.RCTContext.getApplicationContext();
                DownloadManager dm = (DownloadManager) appCtx.getSystemService(Context.DOWNLOAD_SERVICE);
                downloadManagerId = dm.enqueue(req);
                androidDownloadManagerTaskTable.put(taskId, Long.valueOf(downloadManagerId));
                appCtx.registerReceiver(this, new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
                return;
            }

        }

        // find cached result if `key` property exists
        String cacheKey = this.taskId;
        String ext = this.options.appendExt.isEmpty() ? "" : "." + this.options.appendExt;

        if (this.options.key != null) {
            cacheKey = ReactNativeBlobUtilUtils.getMD5(this.options.key);
            if (cacheKey == null) {
                cacheKey = this.taskId;
            }

            File file = new File(ReactNativeBlobUtilFS.getTmpPath(cacheKey) + ext);

            if (file.exists()) {
                callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, file.getAbsolutePath());
                return;
            }
        }

        if (this.options.path != null)
            this.destPath = this.options.path;
        else if (this.options.fileCache)
            this.destPath = ReactNativeBlobUtilFS.getTmpPath(cacheKey) + ext;


        OkHttpClient.Builder clientBuilder;

        try {
            // use trusty SSL socket
            if (this.options.trusty) {
                clientBuilder = ReactNativeBlobUtilUtils.getUnsafeOkHttpClient(client);
            } else {
                clientBuilder = client.newBuilder();
            }

            // wifi only, need ACCESS_NETWORK_STATE permission
            // and API level >= 21
            if (this.options.wifiOnly) {

                boolean found = false;

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                    ConnectivityManager connectivityManager = (ConnectivityManager) ReactNativeBlobUtil.RCTContext.getSystemService(ReactNativeBlobUtil.RCTContext.CONNECTIVITY_SERVICE);
                    Network[] networks = connectivityManager.getAllNetworks();

                    for (Network network : networks) {

                        NetworkInfo netInfo = connectivityManager.getNetworkInfo(network);
                        NetworkCapabilities caps = connectivityManager.getNetworkCapabilities(network);

                        if (caps == null || netInfo == null) {
                            continue;
                        }

                        if (!netInfo.isConnected()) {
                            continue;
                        }

                        if (caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
                            clientBuilder.proxy(Proxy.NO_PROXY);
                            clientBuilder.socketFactory(network.getSocketFactory());
                            found = true;
                            break;

                        }
                    }

                    if (!found) {
                        callback.invoke("No available WiFi connections.", null, null);
                        releaseTaskResource();
                        return;
                    }
                } else {
                    ReactNativeBlobUtilUtils.emitWarningEvent("ReactNativeBlobUtil: wifiOnly was set, but SDK < 21. wifiOnly was ignored.");
                }
            }

            final Request.Builder builder = new Request.Builder();
            try {
                builder.url(new URL(url));
            } catch (MalformedURLException e) {
                e.printStackTrace();
            }

            HashMap<String, String> mheaders = new HashMap<>();
            // set headers
            if (headers != null) {
                ReadableMapKeySetIterator it = headers.keySetIterator();
                while (it.hasNextKey()) {
                    String key = it.nextKey();
                    String value = headers.getString(key);
                    if (key.equalsIgnoreCase("RNFB-Response")) {
                        if (value.equalsIgnoreCase("base64"))
                            responseFormat = ResponseFormat.BASE64;
                        else if (value.equalsIgnoreCase("utf8"))
                            responseFormat = ResponseFormat.UTF8;
                    } else {
                        builder.header(key.toLowerCase(Locale.ROOT), value);
                        mheaders.put(key.toLowerCase(Locale.ROOT), value);
                    }
                }
            }

            if (method.equalsIgnoreCase("post") || method.equalsIgnoreCase("put") || method.equalsIgnoreCase("patch")) {
                String cType = getHeaderIgnoreCases(mheaders, "Content-Type").toLowerCase(Locale.ROOT);

                if (rawRequestBodyArray != null) {
                    requestType = RequestType.Form;
                } else if (cType.isEmpty()) {
                    if (!cType.equalsIgnoreCase("")) {
                        builder.header("Content-Type", "application/octet-stream");
                    }
                    requestType = RequestType.SingleFile;
                }
                if (rawRequestBody != null) {
                    if (rawRequestBody.startsWith(ReactNativeBlobUtilConst.FILE_PREFIX)
                            || rawRequestBody.startsWith(ReactNativeBlobUtilConst.CONTENT_PREFIX)) {
                        requestType = RequestType.SingleFile;
                    } else if (cType.toLowerCase(Locale.ROOT).contains(";base64") || cType.toLowerCase(Locale.ROOT).startsWith("application/octet")) {
                        cType = cType.replace(";base64", "").replace(";BASE64", "");
                        if (mheaders.containsKey("content-type"))
                            mheaders.put("content-type", cType);
                        if (mheaders.containsKey("Content-Type"))
                            mheaders.put("Content-Type", cType);
                        requestType = RequestType.SingleFile;
                    } else {
                        requestType = RequestType.AsIs;
                    }
                }
            } else {
                requestType = RequestType.WithoutBody;
            }

            boolean isChunkedRequest = getHeaderIgnoreCases(mheaders, "Transfer-Encoding").equalsIgnoreCase("chunked");

            // set request body
            switch (requestType) {
                case SingleFile:
                    requestBody = new ReactNativeBlobUtilBody(taskId)
                            .chunkedEncoding(isChunkedRequest)
                            .setRequestType(requestType)
                            .setBody(rawRequestBody)
                            .setMIME(MediaType.parse(getHeaderIgnoreCases(mheaders, "content-type")));
                    builder.method(method, requestBody);
                    break;
                case AsIs:
                    requestBody = new ReactNativeBlobUtilBody(taskId)
                            .chunkedEncoding(isChunkedRequest)
                            .setRequestType(requestType)
                            .setBody(rawRequestBody)
                            .setMIME(MediaType.parse(getHeaderIgnoreCases(mheaders, "content-type")));
                    builder.method(method, requestBody);
                    break;
                case Form:
                    String boundary = "ReactNativeBlobUtil-" + taskId;
                    requestBody = new ReactNativeBlobUtilBody(taskId)
                            .chunkedEncoding(isChunkedRequest)
                            .setRequestType(requestType)
                            .setBody(rawRequestBodyArray)
                            .setMIME(MediaType.parse("multipart/form-data; boundary=" + boundary));
                    builder.method(method, requestBody);
                    break;

                case WithoutBody:
                    if (method.equalsIgnoreCase("post") || method.equalsIgnoreCase("put") || method.equalsIgnoreCase("patch")) {
                        builder.method(method, RequestBody.create(null, new byte[0]));
                    } else
                        builder.method(method, null);
                    break;
            }

            // #156 fix cookie issue
            final Request req = builder.build();
            clientBuilder.addNetworkInterceptor(new Interceptor() {
                @NonNull
                @Override
                public Response intercept(@NonNull Chain chain) throws IOException {
                    redirects.add(chain.request().url().toString());
                    return chain.proceed(chain.request());
                }
            });
            // Add request interceptor for upload progress event
            clientBuilder.addInterceptor(new Interceptor() {
                @NonNull
                @Override
                public Response intercept(@NonNull Chain chain) throws IOException {
                    Response originalResponse = null;
                    try {
                        originalResponse = chain.proceed(req);
                        ResponseBody extended;
                        switch (responseType) {
                            case KeepInMemory:
                                extended = new ReactNativeBlobUtilDefaultResp(
                                        ReactNativeBlobUtil.RCTContext,
                                        taskId,
                                        originalResponse.body(),
                                        options.increment);
                                break;
                            case FileStorage:
                                extended = new ReactNativeBlobUtilFileResp(
                                        ReactNativeBlobUtil.RCTContext,
                                        taskId,
                                        originalResponse.body(),
                                        destPath,
                                        options.overwrite);
                                break;
                            default:
                                extended = new ReactNativeBlobUtilDefaultResp(
                                        ReactNativeBlobUtil.RCTContext,
                                        taskId,
                                        originalResponse.body(),
                                        options.increment);
                                break;
                        }
                        return originalResponse.newBuilder().body(extended).build();
                    } catch (SocketException e) {
                        timeout = true;
                        if (originalResponse != null) {
                            originalResponse.close();
                        }
                    } catch (SocketTimeoutException e) {
                        timeout = true;
                        if (originalResponse != null) {
                            originalResponse.close();
                        }
                        //ReactNativeBlobUtilUtils.emitWarningEvent("ReactNativeBlobUtil error when sending request : " + e.getLocalizedMessage());
                    } catch (Exception ex) {
                        if (originalResponse != null) {
                            originalResponse.close();
                        }
                    }

                    return chain.proceed(chain.request());
                }
            });


            if (options.timeout >= 0) {
                clientBuilder.connectTimeout(options.timeout, TimeUnit.MILLISECONDS);
                clientBuilder.readTimeout(options.timeout, TimeUnit.MILLISECONDS);
            }

            clientBuilder.connectionPool(pool);
            clientBuilder.retryOnConnectionFailure(false);
            clientBuilder.followRedirects(options.followRedirect);
            clientBuilder.followSslRedirects(options.followRedirect);
            clientBuilder.retryOnConnectionFailure(true);

            OkHttpClient client = enableTls12OnPreLollipop(clientBuilder).build();

            Call call = client.newCall(req);
            taskTable.put(taskId, call);
            call.enqueue(new okhttp3.Callback() {

                @Override
                public void onFailure(@NonNull Call call, @NonNull IOException e) {
                    cancelTask(taskId);
                    if (respInfo == null) {
                        respInfo = Arguments.createMap();
                    }

                    // check if this error caused by socket timeout
                    if (e.getClass().equals(SocketTimeoutException.class)) {
                        respInfo.putBoolean("timeout", true);
                        callback.invoke("The request timed out.", null, null);
                    } else
                        callback.invoke(e.getLocalizedMessage(), null, null);
                    releaseTaskResource();
                }

                @Override
                public void onResponse(@NonNull Call call, @NonNull Response response) throws IOException {
                    ReadableMap notifyConfig = options.addAndroidDownloads;
                    // Download manager settings
                    if (notifyConfig != null) {
                        String title = "", desc = "", mime = "text/plain";
                        boolean scannable = false, notification = false;
                        if (notifyConfig.hasKey("title"))
                            title = options.addAndroidDownloads.getString("title");
                        if (notifyConfig.hasKey("description"))
                            desc = notifyConfig.getString("description");
                        if (notifyConfig.hasKey("mime"))
                            mime = notifyConfig.getString("mime");
                        if (notifyConfig.hasKey("mediaScannable"))
                            scannable = notifyConfig.getBoolean("mediaScannable");
                        if (notifyConfig.hasKey("notification"))
                            notification = notifyConfig.getBoolean("notification");
                        DownloadManager dm = (DownloadManager) ReactNativeBlobUtil.RCTContext.getSystemService(ReactNativeBlobUtil.RCTContext.DOWNLOAD_SERVICE);
                        dm.addCompletedDownload(title, desc, scannable, mime, destPath, contentLength, notification);
                    }

                    done(response);
                }
            });


        } catch (Exception error) {
            error.printStackTrace();
            releaseTaskResource();
            callback.invoke("ReactNativeBlobUtil request error: " + error.getMessage() + error.getCause());
        }
    }

    /**
     * Remove cached information of the HTTP task
     */
    private void releaseTaskResource() {
        if (taskTable.containsKey(taskId))
            taskTable.remove(taskId);
        if (androidDownloadManagerTaskTable.containsKey(taskId))
            androidDownloadManagerTaskTable.remove(taskId);
        if (uploadProgressReport.containsKey(taskId))
            uploadProgressReport.remove(taskId);
        if (progressReport.containsKey(taskId))
            progressReport.remove(taskId);
        if (requestBody != null)
            requestBody.clearRequestBody();
    }

    /**
     * Send response data back to javascript context.
     *
     * @param resp OkHttp response object
     */
    private void done(Response resp) {
        boolean isBlobResp = isBlobResponse(resp);
        emitStateEvent(getResponseInfo(resp, isBlobResp));
        switch (responseType) {
            case KeepInMemory:
                try {
                    // For XMLHttpRequest, automatic response data storing strategy, when response
                    // data is considered as binary data, write it to file system
                    if (isBlobResp && options.auto) {
                        String dest = ReactNativeBlobUtilFS.getTmpPath(taskId);
                        InputStream ins = resp.body().byteStream();
                        FileOutputStream os = new FileOutputStream(new File(dest));
                        int read;
                        byte[] buffer = new byte[10240];
                        while ((read = ins.read(buffer)) != -1) {
                            os.write(buffer, 0, read);
                        }
                        ins.close();
                        os.flush();
                        os.close();
                        callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, dest);
                    }
                    // response data directly pass to JS context as string.
                    else {
                        byte[] b = resp.body().bytes();
                        if (responseFormat == ResponseFormat.BASE64) {
                            callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_BASE64, android.util.Base64.encodeToString(b, Base64.NO_WRAP));
                            return;
                        }
                        try {
                            // Attempt to decode the incoming response data to determine whether it contains a valid UTF8 string
                            Charset charSet = Charset.forName("UTF-8");
                            CharsetDecoder decoder = charSet.newDecoder();
                            decoder.decode(ByteBuffer.wrap(b));
                            // If the data contains invalid characters the following lines will be skipped.
                            String utf8 = new String(b, charSet);
                            callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_UTF8, utf8);
                        }
                        // This usually means the data contains invalid unicode characters but still valid data,
                        // it's binary data, so send it as a normal string
                        catch (CharacterCodingException ignored) {
                            if (responseFormat == ResponseFormat.UTF8) {
                                String utf8 = new String(b);
                                callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_UTF8, utf8);
                            } else {
                                callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_BASE64, android.util.Base64.encodeToString(b, Base64.NO_WRAP));
                            }
                        }
                    }
                } catch (IOException e) {
                    callback.invoke("ReactNativeBlobUtil failed to encode response data to BASE64 string.", null);
                }
                break;
            case FileStorage:
                ResponseBody responseBody = resp.body();

                try {
                    // In order to write response data to `destPath` we have to invoke this method.
                    // It uses customized response body which is able to report download progress
                    // and write response data to destination path.
                    responseBody.bytes();
                } catch (Exception ignored) {
//                    ignored.printStackTrace();
                }

                ReactNativeBlobUtilFileResp ReactNativeBlobUtilFileResp;

                try {
                    ReactNativeBlobUtilFileResp = (ReactNativeBlobUtilFileResp) responseBody;
                } catch (ClassCastException ex) {
                    // unexpected response type
                    if (responseBody != null) {
                        String responseBodyString = null;
                        try {
                            boolean isBufferDataExists = responseBody.source().buffer().size() > 0;
                            boolean isContentExists = responseBody.contentLength() > 0;
                            if (isBufferDataExists && isContentExists) {
                                responseBodyString = responseBody.string();
                            }
                        } catch (IOException exception) {
                            exception.printStackTrace();
                        }
                        callback.invoke("Unexpected FileStorage response file: " + responseBodyString, null);
                    } else {
                        callback.invoke("Unexpected FileStorage response with no file.", null);
                    }
                    return;
                }

                if (ReactNativeBlobUtilFileResp != null && !ReactNativeBlobUtilFileResp.isDownloadComplete()) {
                    callback.invoke("Download interrupted.", null);
                } else {
                    this.destPath = this.destPath.replace("?append=true", "");
                    callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, this.destPath);
                }

                break;
            default:
                try {
                    callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_UTF8, new String(resp.body().bytes(), "UTF-8"));
                } catch (IOException e) {
                    callback.invoke("ReactNativeBlobUtil failed to encode response data to UTF8 string.", null);
                }
                break;
        }
//        if(!resp.isSuccessful())
        resp.body().close();
        releaseTaskResource();
    }

    /**
     * Invoke this method to enable download progress reporting.
     *
     * @param taskId Task ID of the HTTP task.
     * @return Task ID of the target task
     */
    public static ReactNativeBlobUtilProgressConfig getReportProgress(String taskId) {
        if (!progressReport.containsKey(taskId)) return null;
        return progressReport.get(taskId);
    }

    /**
     * Invoke this method to enable download progress reporting.
     *
     * @param taskId Task ID of the HTTP task.
     * @return Task ID of the target task
     */
    public static ReactNativeBlobUtilProgressConfig getReportUploadProgress(String taskId) {
        if (!uploadProgressReport.containsKey(taskId)) return null;
        return uploadProgressReport.get(taskId);
    }

    /**
     * Create response information object, contains status code, headers, etc.
     *
     * @param resp       Response object
     * @param isBlobResp If the response is binary data
     * @return Get RCT bridge object contains response information.
     */
    private WritableMap getResponseInfo(Response resp, boolean isBlobResp) {
        WritableMap info = Arguments.createMap();
        info.putInt("status", resp.code());
        info.putString("state", "2");
        info.putString("taskId", this.taskId);
        info.putBoolean("timeout", timeout);
        WritableMap headers = Arguments.createMap();
        for (int i = 0; i < resp.headers().size(); i++) {
            headers.putString(resp.headers().name(i), resp.headers().value(i));
        }
        WritableArray redirectList = Arguments.createArray();
        for (String r : redirects) {
            redirectList.pushString(r);
        }
        info.putArray("redirects", redirectList);
        info.putMap("headers", headers);
        Headers h = resp.headers();
        if (isBlobResp) {
            info.putString("respType", "blob");
        } else if (getHeaderIgnoreCases(h, "content-type").equalsIgnoreCase("text/")) {
            info.putString("respType", "text");
        } else if (getHeaderIgnoreCases(h, "content-type").contains("application/json")) {
            info.putString("respType", "json");
        } else {
            info.putString("respType", "");
        }
        return info;
    }

    /**
     * Check if response data is binary data.
     *
     * @param resp OkHttp response.
     * @return If the response data contains binary bytes
     */
    private boolean isBlobResponse(Response resp) {
        Headers h = resp.headers();
        String ctype = getHeaderIgnoreCases(h, "Content-Type");
        boolean isText = !ctype.equalsIgnoreCase("text/");
        boolean isJSON = !ctype.equalsIgnoreCase("application/json");
        boolean isCustomBinary = false;
        if (options.binaryContentTypes != null) {
            for (int i = 0; i < options.binaryContentTypes.size(); i++) {
                if (ctype.toLowerCase(Locale.ROOT).contains(options.binaryContentTypes.getString(i).toLowerCase(Locale.ROOT))) {
                    isCustomBinary = true;
                    break;
                }
            }
        }
        return (!(isJSON || isText)) || isCustomBinary;
    }

    private String getHeaderIgnoreCases(Headers headers, String field) {
        String val = headers.get(field);
        if (val != null) return val;
        return headers.get(field.toLowerCase(Locale.ROOT)) == null ? "" : headers.get(field.toLowerCase(Locale.ROOT));
    }

    private String getHeaderIgnoreCases(HashMap<String, String> headers, String field) {
        String val = headers.get(field);
        if (val != null) return val;
        String lowerCasedValue = headers.get(field.toLowerCase(Locale.ROOT));
        return lowerCasedValue == null ? "" : lowerCasedValue;
    }

    private void emitStateEvent(WritableMap args) {
        ReactNativeBlobUtil.RCTContext.getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter.class)
                .emit(ReactNativeBlobUtilConst.EVENT_HTTP_STATE, args);
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(action)) {
            Context appCtx = ReactNativeBlobUtil.RCTContext.getApplicationContext();
            long id = intent.getExtras().getLong(DownloadManager.EXTRA_DOWNLOAD_ID);
            if (id == this.downloadManagerId) {
                releaseTaskResource(); // remove task ID from task map

                DownloadManager.Query query = new DownloadManager.Query();
                query.setFilterById(downloadManagerId);
                DownloadManager dm = (DownloadManager) appCtx.getSystemService(Context.DOWNLOAD_SERVICE);
                dm.query(query);
                Cursor c = dm.query(query);
                // #236 unhandled null check for DownloadManager.query() return value
                if (c == null) {
                    this.callback.invoke("Download manager failed to download from  " + this.url + ". Query was unsuccessful ", null, null);
                    return;
                }

                String filePath = null;
                try {
                    // the file exists in media content database
                    if (c.moveToFirst()) {
                        // #297 handle failed request
                        int statusCode = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
                        if (statusCode == DownloadManager.STATUS_FAILED) {
                            this.callback.invoke("Download manager failed to download from  " + this.url + ". Status Code = " + statusCode, null, null);
                            return;
                        }
                        String contentUri = c.getString(c.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI));
                        if (contentUri != null) {
                            Uri uri = Uri.parse(contentUri);
                            Cursor cursor = appCtx.getContentResolver().query(uri, new String[]{android.provider.MediaStore.Files.FileColumns.DATA}, null, null, null);
                            // use default destination of DownloadManager
                            if (cursor != null) {
                                cursor.moveToFirst();
                                filePath = cursor.getString(0);
                                cursor.close();
                            }
                        }
                    }
                } finally {
                    if (c != null) {
                        c.close();
                    }
                }

                // When the file is not found in media content database, check if custom path exists
                if (options.addAndroidDownloads.hasKey("path")) {
                    try {
                        String customDest = options.addAndroidDownloads.getString("path");
                        boolean exists = new File(customDest).exists();
                        if (!exists)
                            throw new Exception("Download manager download failed, the file does not downloaded to destination.");
                        else
                            this.callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, customDest);

                    } catch (Exception ex) {
                        ex.printStackTrace();
                        this.callback.invoke(ex.getLocalizedMessage(), null);
                    }
                } else {
                    if (filePath == null)
                        this.callback.invoke("Download manager could not resolve downloaded file path.", ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, null);
                    else
                        this.callback.invoke(null, ReactNativeBlobUtilConst.RNFB_RESPONSE_PATH, filePath);
                }

            }
        }
    }

    public static OkHttpClient.Builder enableTls12OnPreLollipop(OkHttpClient.Builder client) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN && Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT) {
            try {
                // Code from https://github.com/square/okhttp/issues/2372#issuecomment-244807676
                SSLContext sc = SSLContext.getInstance("TLSv1.2");
                sc.init(null, null, null);
                client.sslSocketFactory(new Tls12SocketFactory(sc.getSocketFactory()));

                ConnectionSpec cs = new ConnectionSpec.Builder(ConnectionSpec.MODERN_TLS)
                        .tlsVersions(TlsVersion.TLS_1_2)
                        .build();

                List<ConnectionSpec> specs = new ArrayList<>();
                specs.add(cs);
                specs.add(ConnectionSpec.COMPATIBLE_TLS);
                specs.add(ConnectionSpec.CLEARTEXT);

                client.connectionSpecs(specs);
            } catch (Exception exc) {
                FLog.e("OkHttpTLSCompat", "Error while setting TLS 1.2", exc);
            }
        }

        return client;
    }


}
