package vision.applicacionnativa;


import static android.Manifest.permission.CAMERA;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.graphics.YuvImage;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.OutputConfiguration;
import android.hardware.camera2.params.SessionConfiguration;
import android.media.Image;
import android.media.ImageReader;
import android.os.Bundle;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.List;


import android.widget.ImageView;


import java.io.ByteArrayOutputStream;
import java.io.IOException;

import okhttp3.Call;
import okhttp3.Callback;
import okhttp3.MediaType;
import okhttp3.MultipartBody;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

import vision.applicacionnativa.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private TextureView textureView;
    private CameraCaptureSession myCameraCaptureSession;
    private String stringCameraID;
    private CameraManager cameraManager;
    private CameraDevice myCameraDevice;
    private CaptureRequest.Builder captureRequestBuilder;
    private ImageView videoStreaming;
    //Coordenadas de los objetos a detectar
    int[][] coordenadas_objetos = {
        {0, 0},
        {0, 0},
    };


    static {
        System.loadLibrary("applicacionnativa");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        videoStreaming = findViewById(R.id.imageView);

        ActivityCompat.requestPermissions(this, new String[]{CAMERA}, PackageManager.PERMISSION_GRANTED);
        textureView = findViewById(R.id.textureView);
        cameraManager = (CameraManager) getSystemService(CAMERA_SERVICE);
        startCamera();
    }

    private CameraDevice.StateCallback stateCallback = new CameraDevice.StateCallback() {
        @Override
        public void onOpened(@NonNull CameraDevice cameraDevice) {
            myCameraDevice = cameraDevice;
        }
        @Override
        public void onDisconnected(@NonNull CameraDevice cameraDevice) {
            myCameraDevice.close();
        }
        @Override
        public void onError(@NonNull CameraDevice cameraDevice, int i) {
            myCameraDevice.close();
            myCameraDevice = null;
        }
    };

    private void startCamera() {
        try {
            stringCameraID = cameraManager.getCameraIdList()[0];

            if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
                return;
            }
            cameraManager.openCamera(stringCameraID, stateCallback, null);
        } catch (CameraAccessException e) {
            throw new RuntimeException(e);
        }
    }

    private ImageReader imageReader;

    public void buttonStartCamera(View view) {
        SurfaceTexture surfaceTexture = textureView.getSurfaceTexture();
        Surface surface = new Surface(surfaceTexture);

        // Configuramos el ImageReader para capturar imágenes en formato YUV_420_888
        imageReader = ImageReader.newInstance(textureView.getWidth(), textureView.getHeight(), ImageFormat.YUV_420_888, 2);
        imageReader.setOnImageAvailableListener(imageAvailableListener, null);

        try {
            captureRequestBuilder = myCameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            captureRequestBuilder.addTarget(surface);
            captureRequestBuilder.addTarget(imageReader.getSurface()); // Agregamos el ImageReader como target

            List<OutputConfiguration> outputConfigurations = Arrays.asList(
                    new OutputConfiguration(surface),
                    new OutputConfiguration(imageReader.getSurface())
            );

            SessionConfiguration sessionConfiguration = new SessionConfiguration(
                    SessionConfiguration.SESSION_REGULAR,
                    outputConfigurations,
                    getMainExecutor(),
                    new CameraCaptureSession.StateCallback() {
                        @Override
                        public void onConfigured(@NonNull CameraCaptureSession cameraCaptureSession) {
                            myCameraCaptureSession = cameraCaptureSession;
                            try {
                                myCameraCaptureSession.setRepeatingRequest(captureRequestBuilder.build(), null, null);
                            } catch (CameraAccessException e) {
                                throw new RuntimeException(e);
                            }
                        }

                        @Override
                        public void onConfigureFailed(@NonNull CameraCaptureSession cameraCaptureSession) {
                            myCameraCaptureSession = null;
                        }
                    }
            );

            myCameraDevice.createCaptureSession(sessionConfiguration);

        } catch (CameraAccessException e) {
            throw new RuntimeException(e);
        }
    }

    public void buttonStopCamera(View view){
        try {
            myCameraCaptureSession.abortCaptures();
        } catch (CameraAccessException e) {
            throw new RuntimeException(e);
        }
    }

    private final ImageReader.OnImageAvailableListener imageAvailableListener = new ImageReader.OnImageAvailableListener() {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Image image = null;
            try {
                image = reader.acquireLatestImage();
                if (image != null) {
                    // Convertimos la imagen a Bitmap
                    Bitmap bitmap = imageToBitmap(image);
                    image.close();
                    // Envíamos el Bitmap al servidor Flask
                    sendBitmapToServer(bitmap);

                    android.graphics.Bitmap bOut = bitmap.copy(bitmap.getConfig(), true);
                    graficarObjetos(bitmap, bOut, coordenadas_objetos);

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            videoStreaming.setImageBitmap(bOut);
                        }
                    });

                }
            } catch (Exception e) {
                e.printStackTrace();
                if (image != null) {
                    image.close();
                }
            }
        }
    };


    private Bitmap imageToBitmap(Image image) {
        // Obtenemos los planos de YUV
        Image.Plane[] planes = image.getPlanes();
        ByteBuffer bufferY = planes[0].getBuffer(); // Plan Y
        ByteBuffer bufferU = planes[1].getBuffer(); // Plan U
        ByteBuffer bufferV = planes[2].getBuffer(); // Plan V

        // Obtenemos los tamaños de cada buffer
        int ySize = bufferY.remaining();
        int uSize = bufferU.remaining();
        int vSize = bufferV.remaining();

        // Creamos un array para los datos NV21
        byte[] nv21Bytes = new byte[ySize + uSize + vSize / 2]; // NV21 tiene una resolución de croma 1/2

        // Copiamos el buffer Y
        bufferY.get(nv21Bytes, 0, ySize);

        // Copiar los planos U y V en formato intercalado
        int uvIndex = ySize;
        for (int i = 0; i < uSize; i++) {
            if (uvIndex + 2 * i + 1 < nv21Bytes.length) {
                nv21Bytes[uvIndex + 2 * i] = bufferV.get(i);
                nv21Bytes[uvIndex + 2 * i + 1] = bufferU.get(i);
            }
        }

        // Creamos un YuvImage con los datos NV21
        YuvImage yuvImage = new YuvImage(nv21Bytes, ImageFormat.NV21, image.getWidth(), image.getHeight(), null);

        // Creamos un ByteArrayOutputStream para guardar la imagen JPEG
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        yuvImage.compressToJpeg(new Rect(0, 0, image.getWidth(), image.getHeight()), 100, outputStream);


        // Decodificamos los datos JPEG a un Bitmap
        byte[] jpegData = outputStream.toByteArray();
        Bitmap bitmap = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length);

        return bitmap;
    }

    private void sendBitmapToServer(Bitmap bitmap) {

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
        byte[] imageBytes = outputStream.toByteArray();

        OkHttpClient client = new OkHttpClient();
        RequestBody requestBody = new MultipartBody.Builder()
                .setType(MultipartBody.FORM)
                .addFormDataPart("image", "frame.jpg",
                        RequestBody.create(MediaType.parse("image/jpeg"), imageBytes))
                .build();

        Request request = new Request.Builder()
                .url("http://192.168.18.162:5000/upload3")
                .post(requestBody)
                .build();

        client.newCall(request).enqueue(new Callback() {
            @Override
            public void onFailure(Call call, IOException e) {
                e.printStackTrace();
            }

            @Override
            public void onResponse(Call call, Response response) throws IOException {
                if (response.isSuccessful()) {

                    String responseString = response.body().string();

                    JsonObject jsonObject = JsonObject.fromJson(responseString);

                    coordenadas_objetos = coordenadas(jsonObject);

                } else {
                    System.out.println("Error al procesar la imagen en el servidor.");
                }
            }

        });

    }

    public native int[][] coordenadas(JsonObject jsonObject);

    public native void graficarObjetos(android.graphics.Bitmap in, android.graphics.Bitmap out, int[][] coordinates);


}

