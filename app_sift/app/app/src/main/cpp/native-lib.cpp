#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/bitmap.h>
#include <vector>
#include <android/log.h>

using namespace std;
using namespace cv;

Mat logo;


struct Puntos {
    double pt[2];
    double size;
    double angle;
    double response;
    int octave;
    int class_id;
};

vector<Puntos> points;

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean needUnPremultiplyAlpha){
    AndroidBitmapInfo  info;
    void*       pixels = 0;

    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        //jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
        //if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}


void matToBitmap(JNIEnv * env, cv::Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo  info;
    void*              pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width ==
                                                                         (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha) cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        //jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
        //if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}


extern "C" JNIEXPORT jdoubleArray JNICALL
Java_vision_applicacionnativa_MainActivity_coordenadas(JNIEnv* env, jobject /*this*/, jobject jsonObject){


    // Obtemos la lista de puntos desde el objeto Java
    jclass jsonObjectClass = env->GetObjectClass(jsonObject);
    jfieldID pointsFieldID = env->GetFieldID(jsonObjectClass, "points", "Ljava/util/List;");
    jobject pointsList = env->GetObjectField(jsonObject, pointsFieldID);

    // Convertimos el List de Java en un arreglo de C++
    jclass listClass = env->GetObjectClass(pointsList);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jint size = env->CallIntMethod(pointsList, sizeMethod);

    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

    vector<KeyPoint> keypoints_filtrados;

    // Iteramos a través de los keypoints recibidos y agregarlos a la lista de keypoints filtrados
    for (int i = 0; i < size; i++) {
        jobject pointObj = env->CallObjectMethod(pointsList, getMethod, i);

        // Obtener los atributos de cada KeyPoint
        jclass pointClass = env->GetObjectClass(pointObj);
        jfieldID ptField = env->GetFieldID(pointClass, "pt", "[D");
        jfieldID sizeField = env->GetFieldID(pointClass, "size", "D");
        jfieldID angleField = env->GetFieldID(pointClass, "angle", "D");
        jfieldID responseField = env->GetFieldID(pointClass, "response", "D");
        jfieldID octaveField = env->GetFieldID(pointClass, "octave", "I");
        jfieldID classIdField = env->GetFieldID(pointClass, "class_id", "I");

        jdoubleArray ptArray = (jdoubleArray) env->GetObjectField(pointObj, ptField);
        jdouble* pt = env->GetDoubleArrayElements(ptArray, 0);

        double sizeValue = env->GetDoubleField(pointObj, sizeField);
        double angleValue = env->GetDoubleField(pointObj, angleField);
        double responseValue = env->GetDoubleField(pointObj, responseField);
        jint octaveValue = env->GetIntField(pointObj, octaveField);
        jint classIdValue = env->GetIntField(pointObj, classIdField);

        // Creamos KeyPoint con los valores recibidos desde Java
        KeyPoint keypoint(pt[0], pt[1], sizeValue, angleValue, responseValue, octaveValue, classIdValue);
        keypoints_filtrados.push_back(keypoint);

        // Liberamos el arreglo de puntos después de usarlo
        env->ReleaseDoubleArrayElements(ptArray, pt, 0);
    }

    // Creamos un vector para almacenar las coordenadas (x, y) de los keypoints
    std::vector<cv::Point2f> pts;
    for (const auto& kp : keypoints_filtrados) {
        pts.push_back(kp.pt);  // Extraemos el punto de cada keypoint
    }

    // Encontramos el rectángulo que rodea todos los puntos clave
    float x_min = std::min_element(pts.begin(), pts.end(),
                                   [](const cv::Point2f& a, const cv::Point2f& b) {
                                       return a.x < b.x;
                                   })->x;
    float y_min = std::min_element(pts.begin(), pts.end(),
                                   [](const cv::Point2f& a, const cv::Point2f& b) {
                                       return a.y < b.y;
                                   })->y;
    float x_max = std::max_element(pts.begin(), pts.end(),
                                   [](const cv::Point2f& a, const cv::Point2f& b) {
                                       return a.x < b.x;
                                   })->x;
    float y_max = std::max_element(pts.begin(), pts.end(),
                                   [](const cv::Point2f& a, const cv::Point2f& b) {
                                       return a.y < b.y;
                                   })->y;


    jdoubleArray resultArray = env->NewDoubleArray(4);

    jdouble values[] = {x_min, y_min, x_max, y_max};

    env->SetDoubleArrayRegion(resultArray, 0, 4, values);  // Copiar los valores en el arreglo


    return resultArray;
}



extern "C" JNIEXPORT void JNICALL
Java_vision_applicacionnativa_MainActivity_graficar(JNIEnv* env, jobject /*this*/, jobject bitmapIn, jobject bitmapOut, jdoubleArray doubleArray1, jdoubleArray doubleArray2){

    Mat frame;

    // Convertimos el bitmap de entrada a Mat
    bitmapToMat(env, bitmapIn, frame, false);

    jdouble* arrayElements1 = env->GetDoubleArrayElements(doubleArray1, nullptr);
    jdouble* arrayElements2 = env->GetDoubleArrayElements(doubleArray2, nullptr);


    //************ TRABAJAR DESDE AQUÍ SOLO CON FRAME ************

    // Creamos una copia de la imagen para dibujar sobre ella
    cv::Mat imagen_con_rectangulo = frame.clone();


    // Dibujamos el rectángulo alrededor de todos los keypoints
    cv::rectangle(imagen_con_rectangulo,
                  cv::Point(arrayElements1[0], arrayElements1[1]),  // Esquina superior izquierda
                  cv::Point(arrayElements1[2], arrayElements1[3]),  // Esquina inferior derecha
                  cv::Scalar(0, 255, 0),
                  2);

    cv::rectangle(imagen_con_rectangulo,
                  cv::Point(arrayElements2[0], arrayElements2[1]),  // Esquina superior izquierda
                  cv::Point(arrayElements2[2], arrayElements2[3]),  // Esquina inferior derecha
                  cv::Scalar(0, 27, 255),
                  2);

    //__android_log_print(ANDROID_LOG_DEBUG, "graficar", "Valor en posición %d: %f", arrayElements[0]);



    // Convertimos la imagen resultante a bitmap

    matToBitmap(env, imagen_con_rectangulo, bitmapOut, false);

}



//Java_vision_applicacionnativa_MainActivity_filtroSketch(JNIEnv* env, jobject /*this*/, jobject bitmapIn, jobject bitmapOut, jobject jsonObject){