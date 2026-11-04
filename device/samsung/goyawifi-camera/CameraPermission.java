// SPDX-License-Identifier: Apache-2.0
public class CameraPermission {
 public static void main(String[] args) throws Exception {
  Object pm=Class.forName("android.app.AppGlobals").getMethod("getPackageManager").invoke(null);
  for(String p:new String[]{"CAMERA","WRITE_EXTERNAL_STORAGE","READ_EXTERNAL_STORAGE"})Class.forName("android.content.pm.IPackageManager").getMethod("grantRuntimePermission",String.class,String.class,int.class).invoke(pm,"org.goya.camera","android.permission."+p,0);
  System.out.println("CAMERA_PERMISSIONS_GRANTED");
 }
}
