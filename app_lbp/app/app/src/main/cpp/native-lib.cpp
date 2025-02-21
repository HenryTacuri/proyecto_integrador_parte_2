#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/bitmap.h>
#include <vector>
#include <android/log.h>

using namespace std;
using namespace cv;


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


extern "C" JNIEXPORT void JNICALL
Java_vision_applicacionnativa_MainActivity_graficarObjetos(JNIEnv* env, jobject /*this*/, jobject bitmapIn, jobject bitmapOut, jobjectArray coordinates) {
    Mat frame;

    // Convertimos el bitmap de entrada a Mat
    bitmapToMat(env, bitmapIn, frame, false);



    jsize rowCount = env->GetArrayLength(coordinates);

    // Creamos un vector de vectores de enteros en C++ (equivalente a int[][] en Java)
    vector<vector<int>> cCoordinates;

    for (jsize i = 0; i < rowCount; ++i) {
        // Obtemos el arreglo de enteros en la posición i
        jintArray rowArray = (jintArray) env->GetObjectArrayElement(coordinates, i);

        // Obtemos el tamaño de la fila (columna)
        jsize colCount = env->GetArrayLength(rowArray);

        // Creamos un vector para almacenar los elementos de la fila
        std::vector<int> rowVector(colCount);

        // Copiamos los elementos del arreglo Java al vector de C++
        env->GetIntArrayRegion(rowArray, 0, colCount, rowVector.data());

        // Agregamos la fila al vector bidimensional
        cCoordinates.push_back(rowVector);

        // Liberamos la referencia local del arreglo de enteros
        env->DeleteLocalRef(rowArray);
    }

    for (const auto& coord : cCoordinates) {
        rectangle(frame, Point(coord[0], coord[1]), Point((coord[0]+ coord[2]), (coord[1]+ coord[3])), Scalar(0, 255, 0), 2);
    }

    matToBitmap(env, frame, bitmapOut, false);
}

extern "C" jobjectArray JNICALL
Java_vision_applicacionnativa_MainActivity_coordenadas(JNIEnv* env, jobject /*this*/, jobject jsonObject) {

    // Obtemos la lista de puntos desde el objeto Java
    jclass jsonObjectClass = env->GetObjectClass(jsonObject);
    jfieldID pointsFieldID = env->GetFieldID(jsonObjectClass, "points", "Ljava/util/List;");
    jobject pointsList = env->GetObjectField(jsonObject, pointsFieldID);

    // Convertimos el List de Java en un arreglo de C++
    jclass listClass = env->GetObjectClass(pointsList);
    jmethodID sizeMethod = env->GetMethodID(listClass, "size", "()I");
    jint size = env->CallIntMethod(pointsList, sizeMethod);

    jmethodID getMethod = env->GetMethodID(listClass, "get", "(I)Ljava/lang/Object;");

    // Vector para almacenar las coordenadas
    vector<vector<int>> coordinates;

    // Iteramos a través de los objetos y obtener las coordenadas
    for (int i = 0; i < size; i++) {
        jobject pointObj = env->CallObjectMethod(pointsList, getMethod, i);

        // Obtenemos los atributos de cada objeto (coordenadas)
        jclass pointClass = env->GetObjectClass(pointObj);
        jfieldID xField = env->GetFieldID(pointClass, "x", "I");
        jfieldID yField = env->GetFieldID(pointClass, "y", "I");
        jfieldID wField = env->GetFieldID(pointClass, "w", "I");
        jfieldID hField = env->GetFieldID(pointClass, "h", "I");

        jint x = env->GetIntField(pointObj, xField);
        jint y = env->GetIntField(pointObj, yField);
        jint w = env->GetIntField(pointObj, wField);
        jint h = env->GetIntField(pointObj, hField);

        // Creamos un objeto cv::Rect con las coordenadas (x, y, w, h)
        coordinates.push_back({x, y, w, h});
    }


    jclass intArrayClass = env->FindClass("[I");

    // Creamos el array de objetos de tipo int[]
    jobjectArray resultArray = env->NewObjectArray(coordinates.size(), intArrayClass, nullptr);

    // Llenaramos el arreglo con los sub-arreglos de enteros
    for (size_t i = 0; i < coordinates.size(); ++i) {
        // Creamos un array de enteros de tamaño igual al de la fila
        jintArray intArray = env->NewIntArray(coordinates[i].size());

        // Copiamos los elementos del vector al arreglo de enteros
        env->SetIntArrayRegion(intArray, 0, coordinates[i].size(), coordinates[i].data());

        // Asignamos el arreglo de enteros al array de objetos
        env->SetObjectArrayElement(resultArray, i, intArray);

        // Liberamos el arreglo temporal de enteros
        env->DeleteLocalRef(intArray);
    }

    return resultArray;
}