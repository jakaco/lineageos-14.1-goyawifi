// SPDX-License-Identifier: Apache-2.0
public class HelloNougat {
    public static void main(String[] args) throws Exception {
        final java.util.concurrent.atomic.AtomicInteger value = new java.util.concurrent.atomic.AtomicInteger();
        Thread worker = new Thread(new Runnable() {
            public void run() { for (int i=1;i<=1000;i++) value.addAndGet(i); }
        });
        worker.start(); worker.join();
        if (value.get()!=500500) throw new AssertionError("thread/atomic sum");
        if (!String.class.getMethod("substring",int.class).invoke("Nougat",3).equals("gat"))
            throw new AssertionError("reflection");
        java.io.File f=new java.io.File("/data/local/tmp/art-probe.txt");
        java.io.FileOutputStream stream=new java.io.FileOutputStream(f);
        stream.write("RAM-only Nougat ART probe\n".getBytes("UTF-8")); stream.close();
        if (f.length()!=26 || !f.delete()) throw new AssertionError("RAM file IO");
        System.gc();
        System.out.println("ANDROID7_ART_PASS sum="+value.get()+" vm="+System.getProperty("java.vm.name")+" version="+System.getProperty("java.vm.version"));
    }
}
