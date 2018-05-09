package org.kde.something;

import android.content.ContentResolver;
import android.content.Intent;
import android.util.Log;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.net.Uri;

import org.qtproject.qt5.android.bindings.QtActivity;

class FileClass
{
    public static native void openUri(String uri);
}

public class OpenFileActivity extends QtActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final Intent bundleIntent = getIntent();
        if (bundleIntent == null)
            return;

        final String action = bundleIntent.getAction();
        Uri uri = bundleIntent.getData();
        if (!uri.getScheme().equals("file")) {
            try {
                ContentResolver resolver = getBaseContext().getContentResolver();
                ParcelFileDescriptor fdObject = resolver.openFileDescriptor(uri, "r");
                uri = Uri.parse("fd:///" + fdObject.detachFd());
            } catch (Exception e) {
                e.printStackTrace();

                //TODO: emit warning that couldn't be opened
                Log.v("Okular", "failed to open");
                return;
            }
        }

        FileClass.openUri(uri.toString());
    }
}
