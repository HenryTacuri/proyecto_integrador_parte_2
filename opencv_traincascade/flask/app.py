from flask import Flask, request, send_file, jsonify
import cv2
import numpy as np
import io
import json


app = Flask(__name__)





def detectar_y_comparar_objeto_1(imagen):
    # Cargamos la imagen y el logo
    logo = cv2.imread('../objeto1.png')

    # Creamos el detector SIFT
    detector = cv2.SIFT_create()

    # Detectamos los keypoints y descriptores de la imagen y del logo
    keypoints_imagen, descriptores_imagen = detector.detectAndCompute(imagen, None)
    keypoints_logo, descriptores_logo = detector.detectAndCompute(logo, None)

    # Creamos el objeto BFMatcher para encontrar las coincidencias
    matcher = cv2.BFMatcher(cv2.NORM_L2, crossCheck=False)

    # Encontramos las mejores coincidencias utilizando knnMatch
    matches = matcher.knnMatch(descriptores_logo, descriptores_imagen, k=2)

    # Filtramos las coincidencias utilizando el ratio de Lowe
    matches_filtrados = []
    ratio = 0.75  # Umbral de filtrado
    for m, n in matches:
        if m.distance < ratio * n.distance:
            matches_filtrados.append(m)

    
    keypoints_imagen_filtrados = [keypoints_imagen[match.trainIdx] for match in matches_filtrados]

    # Convertimos keypoints filtrados a una estructura serializable
    keypoints_imagen_filtrados_json = [{'pt': (kp.pt[0], kp.pt[1]), 'size': kp.size, 'angle': kp.angle, 
                                        'response': kp.response, 'octave': kp.octave, 'class_id': kp.class_id} 
                                       for kp in keypoints_imagen_filtrados]
    

    return jsonify(keypoints_imagen_filtrados_json)


def detectar_y_comparar_objeto_2(imagen):
    # Cargamos la imagen y el logo
    logo = cv2.imread('../objeto2.png')

    # Creamos el detector SIFT
    detector = cv2.SIFT_create()

    # Detectamos los keypoints y descriptores de la imagen y del logo
    keypoints_imagen, descriptores_imagen = detector.detectAndCompute(imagen, None)
    keypoints_logo, descriptores_logo = detector.detectAndCompute(logo, None)

    # Creamos el objeto BFMatcher para encontrar las coincidencias
    matcher = cv2.BFMatcher(cv2.NORM_L2, crossCheck=False)

    # Encontramos las mejores coincidencias utilizando knnMatch
    matches = matcher.knnMatch(descriptores_logo, descriptores_imagen, k=2)

    # Filtramos las coincidencias utilizando el ratio de Lowe
    matches_filtrados = []
    ratio = 0.75  # Umbral de filtrado
    for m, n in matches:
        if m.distance < ratio * n.distance:
            matches_filtrados.append(m)

    
    keypoints_imagen_filtrados = [keypoints_imagen[match.trainIdx] for match in matches_filtrados]

    # Convertimos keypoints filtrados a una estructura serializable
    keypoints_imagen_filtrados_json = [{'pt': (kp.pt[0], kp.pt[1]), 'size': kp.size, 'angle': kp.angle, 
                                        'response': kp.response, 'octave': kp.octave, 'class_id': kp.class_id} 
                                       for kp in keypoints_imagen_filtrados]
    

    return jsonify(keypoints_imagen_filtrados_json)


def detectar_y_comparar_logos(image):

    cascade = cv2.CascadeClassifier('lbp/cascade.xml')

    # Detectamos los objetos en la imagen    
    objects = cascade.detectMultiScale(image, scaleFactor=1.1, minNeighbors=2, minSize=(30, 30))

    # Creamos una lista de coordenadas para los objetos detectados
    coordenadas = []
    for (x, y, w, h) in objects:
        coordenadas.append({"x": int(x), "y": int(y), "w": int(w), "h": int(h)})

    return jsonify({"coordenadas": coordenadas})



@app.route('/upload1', methods=['POST'])
def upload1():
    file = request.files['image']
    npimg = np.frombuffer(file.read(), np.uint8)
    img = cv2.imdecode(npimg, cv2.IMREAD_COLOR)

    respKeyPoints = detectar_y_comparar_objeto_1(img)

    return respKeyPoints


@app.route('/upload2', methods=['POST'])
def upload2():
    file = request.files['image']
    npimg = np.frombuffer(file.read(), np.uint8)
    img = cv2.imdecode(npimg, cv2.IMREAD_COLOR)

    respKeyPoints = detectar_y_comparar_objeto_2(img)


    return respKeyPoints


@app.route('/upload3', methods=['POST'])
def upload3():
    file = request.files['image']
    npimg = np.frombuffer(file.read(), np.uint8)
    img = cv2.imdecode(npimg, cv2.IMREAD_COLOR)

    coordenadas = detectar_y_comparar_logos(img)

    return coordenadas


if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
