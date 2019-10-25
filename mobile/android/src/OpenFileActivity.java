package org.kde.something;

import android.content.ContentResolver;
import android.content.Intent;
import android.util.Log;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.net.Uri;
import android.app.Activity;

import java.io.FileNotFoundException;

import org.qtproject.qt5.android.bindings.QtActivity;

class FileClass
{
    public static native void openUri(String uri);
}

public class OpenFileActivity extends QtActivity
{

    public String contentUrlToFd(String url)
    {
        try {
            ContentResolver resolver = getBaseContext().getContentResolver();
            ParcelFileDescriptor fdObject = resolver.openFileDescriptor(Uri.parse(url), "r");
            return "fd:///" + fdObject.detachFd();
        } catch (FileNotFoundException e) {
            Log.e("Okular", "Cannot find file", e);
        }
        return "";
    }


    private void displayUri(Uri uri)
    {
        if (uri == null)
            return;

        if (!uri.getScheme().equals("file")) {
            try {
                ContentResolver resolver = getBaseContext().getContentResolver();
                ParcelFileDescriptor fdObject = resolver.openFileDescriptor(uri, "r");
                uri = Uri.parse("fd:///" + fdObject.detachFd());
            } catch (Exception e) {
                e.printStackTrace();

                //TODO: emit warning that couldn't be opened
                Log.e("Okular", "failed to open");
                return;
            }
        }

        Log.e("Okular", "opening url: " + uri.toString());
        FileClass.openUri(uri.toString());
    }

    public void handleViewIntent() {
        final Intent bundleIntent = getIntent();
        if (bundleIntent == null)
            return;

        final String action = bundleIntent.getAction();
        Log.v("Okular", "Starting action: " + action);
        if (action == "android.intent.action.VIEW") {
            displayUri(bundleIntent.getData());
        }
    }
}
