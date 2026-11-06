// SPDX-License-Identifier: Apache-2.0
package org.goya.camera;
import android.app.*;
import android.os.*;
import android.Manifest;
import android.content.*;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.media.ExifInterface;
import android.graphics.*;
import android.provider.MediaStore;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;

@SuppressWarnings("deprecation")
public class CameraActivity extends Activity implements SurfaceHolder.Callback {
 private SurfaceView surface;private TextView status;private Button shutter,last;private Camera camera;private boolean resumed,ready,busy;private File latest;private HandlerThread thread;private Handler worker;private final Handler ui=new Handler();
 public void onCreate(Bundle b){super.onCreate(b);thread=new HandlerThread("Camera worker");thread.start();worker=new Handler(thread.getLooper());
  LinearLayout root=new LinearLayout(this);root.setOrientation(1);root.setBackgroundColor(Color.BLACK);
  FrameLayout frame=new FrameLayout(this);surface=new SurfaceView(this);surface.getHolder().addCallback(this);FrameLayout.LayoutParams fp=new FrameLayout.LayoutParams(-1,-1,Gravity.CENTER);frame.addView(surface,fp);root.addView(frame,new LinearLayout.LayoutParams(-1,0,1));
  status=new TextView(this);status.setTextColor(Color.WHITE);status.setPadding(18,10,18,10);status.setText("Starting cameraâ€¦");root.addView(status);
  LinearLayout buttons=new LinearLayout(this);shutter=new Button(this);shutter.setText("Take photo");shutter.setOnClickListener(v->take());buttons.addView(shutter,new LinearLayout.LayoutParams(0,-2,1));last=new Button(this);last.setText("Last photo");last.setOnClickListener(v->showPhoto());buttons.addView(last,new LinearLayout.LayoutParams(0,-2,1));root.addView(buttons);setContentView(root);getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  frame.addOnLayoutChangeListener((v,l,t,r,bt,ol,ot,or,ob)->{int w=r-l,h=bt-t;int sw=Math.min(w,h*3/4),sh=sw*4/3;FrameLayout.LayoutParams p=(FrameLayout.LayoutParams)surface.getLayoutParams();if(p.width!=sw||p.height!=sh){p.width=sw;p.height=sh;surface.setLayoutParams(p);}});
 }
 private boolean permissions(){return checkSelfPermission(Manifest.permission.CAMERA)==PackageManager.PERMISSION_GRANTED&&checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)==PackageManager.PERMISSION_GRANTED;}
 public void onResume(){super.onResume();resumed=true;if(!permissions())requestPermissions(new String[]{Manifest.permission.CAMERA,Manifest.permission.WRITE_EXTERNAL_STORAGE},1);else open();}
 public void onRequestPermissionsResult(int q,String[]p,int[]r){super.onRequestPermissionsResult(q,p,r);if(permissions())open();else text("Camera and photo storage permissions are required.");}
 public void surfaceCreated(SurfaceHolder h){ready=true;open();}public void surfaceChanged(SurfaceHolder h,int f,int w,int ht){}public void surfaceDestroyed(SurfaceHolder h){ready=false;close();}
 private void text(String s){ui.post(()->status.setText(s));}
 private void open(){if(!resumed||!ready||!permissions())return;worker.post(()->{if(camera!=null||!resumed||!ready)return;try{camera=Camera.open(0);Camera.Parameters p=camera.getParameters();p.setPreviewSize(640,480);p.setPictureSize(1600,1200);p.setJpegQuality(90);p.setRotation(90);camera.setParameters(p);camera.setDisplayOrientation(90);camera.setPreviewDisplay(surface.getHolder());camera.setErrorCallback((e,c)->text("Camera error "+e+". Close and reopen Camera."));camera.startPreview();busy=false;text("Rear camera Â· 2 MP photos");}catch(Exception e){text("Camera could not start: "+e.getMessage());release();}});}
 private void take(){worker.post(()->{if(camera==null||busy)return;busy=true;ui.post(()->shutter.setEnabled(false));text("Taking photoâ€¦");try{camera.takePicture(null,null,(bytes,c)->worker.post(()->{try{File dir=new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM),"Camera");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Cannot create photo folder");File f=new File(dir,"IMG_"+new SimpleDateFormat("yyyyMMdd_HHmmss_SSS",Locale.US).format(new Date())+".jpg");try(FileOutputStream out=new FileOutputStream(f)){out.write(bytes);}ContentValues v=new ContentValues();v.put(MediaStore.Images.Media.DATA,f.getAbsolutePath());v.put(MediaStore.Images.Media.MIME_TYPE,"image/jpeg");v.put(MediaStore.Images.Media.DISPLAY_NAME,f.getName());v.put(MediaStore.Images.Media.DATE_TAKEN,System.currentTimeMillis());v.put(MediaStore.Images.Media.ORIENTATION,90);v.put(MediaStore.Images.Media.WIDTH,1600);v.put(MediaStore.Images.Media.HEIGHT,1200);if(getPackageManager().resolveContentProvider("media",0)!=null){try{getContentResolver().insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI,v);}catch(RuntimeException e){android.util.Log.w("GoyaCameraApp","Photo saved; media indexing unavailable",e);}}latest=f;text("Saved "+f.getName()+" Â· 1600 Ã— 1200");}catch(Exception e){text("Photo save failed: "+e.getMessage());}finally{busy=false;ui.post(()->shutter.setEnabled(true));try{if(camera!=null&&resumed&&ready)camera.startPreview();}catch(Exception e){text("Preview restart failed: "+e.getMessage());}}}));}catch(Exception e){busy=false;ui.post(()->shutter.setEnabled(true));text("Capture failed: "+e.getMessage());}});}
 private void showPhoto(){if(latest==null||!latest.exists()){text("Take a photo first.");return;}BitmapFactory.Options o=new BitmapFactory.Options();o.inSampleSize=2;Bitmap b=BitmapFactory.decodeFile(latest.getAbsolutePath(),o);if(b==null){text("Cannot decode saved photo");return;}Matrix m=new Matrix();m.postRotate(90);Bitmap upright=Bitmap.createBitmap(b,0,0,b.getWidth(),b.getHeight(),m,true);ImageView image=new ImageView(this);image.setImageBitmap(upright);image.setAdjustViewBounds(true);new AlertDialog.Builder(this).setTitle("Saved photo").setView(image).setPositiveButton("Close",(d,w)->{}).show();}
 private void release(){if(camera!=null){try{camera.stopPreview();}catch(Exception e){}camera.release();camera=null;}busy=false;ui.post(()->shutter.setEnabled(true));}
 private void close(){worker.post(()->release());}public void onPause(){resumed=false;close();super.onPause();}public void onDestroy(){worker.post(()->thread.quitSafely());super.onDestroy();}
}
