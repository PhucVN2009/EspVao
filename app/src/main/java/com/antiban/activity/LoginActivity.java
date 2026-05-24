package com.antiban.activity;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.graphics.Color;
import android.os.Build;
import android.provider.Settings;
import android.content.Intent;
import android.net.Uri;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.app.AlertDialog;
import android.os.Handler;
import android.graphics.drawable.ColorDrawable;
import android.os.Message;
import android.content.DialogInterface;

import android.widget.EditText;
import android.widget.ImageView;
import android.text.method.PasswordTransformationMethod;
import android.text.method.HideReturnsTransformationMethod;
import android.view.MotionEvent;
import android.annotation.SuppressLint;
import android.widget.TextView;
import android.content.pm.PackageManager;

import android.widget.Toast;
import android.Manifest;
import android.util.Log;

public class LoginActivity extends Activity {

    private String TAG;
    
    static {
        System.loadLibrary("DIPOK_MODS");
    }
    
    private void setLightStatusBar(Activity activity) {
        activity.getWindow().setStatusBarColor(Color.parseColor("#FF121212"));
        activity.getWindow().setNavigationBarColor(Color.parseColor("#FF121212"));
    }
    
    void startPatcher() {
        if (Build.VERSION.SDK_INT >= 23 && !Settings.canDrawOverlays(LoginActivity.this)) {
            Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + LoginActivity.this.getPackageName()));
            startActivityForResult(intent, 123);
        }
    }
    
    private static final String USER = "USER";
    public static String USERKEY;
    public static boolean splashloaded;
    static boolean crackerdetect = false;
    
    
    
     
    public static String a[] = GetKey().split(" ");
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setLightStatusBar(this);
        
		
        startPatcher();
        init();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED) {
                Log.v(TAG, "Permission is granted");
            } else {
                Log.v(TAG,"Permission is revoked");
                
                Toast.makeText(LoginActivity.this,"Please Allow permision storage",Toast.LENGTH_SHORT).show();
                new Handler().postDelayed(new Runnable()  {
                        @Override
                        public void run() {
                            finishAffinity();
                        }
                    }, 2000);
            }
        } else { 
            Log.v(TAG,"Permission is granted");
        }
    }
    
    
    
    private void init(){
        String[] arraySpinner = new String[] { "English","Arab" };
        
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,android.R.layout.simple_spinner_item,arraySpinner);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		
    }
    
    
    
    private static native String Check(Context mContext, String userKey);


    static native String GetKey();
    
}
