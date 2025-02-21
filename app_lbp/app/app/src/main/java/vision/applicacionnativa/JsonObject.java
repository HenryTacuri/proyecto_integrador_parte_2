package vision.applicacionnativa;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

class Point {
    int x, y, w, h;
    // Constructor
    public Point(int x, int y, int w, int h) {
        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
    }

    // Métodos getters
    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public int getW() {
        return w;
    }

    public int getH() {
        return h;
    }
}


public class JsonObject {
    List<Point> points; // Lista de puntos (coordenadas)

    // Constructor
    public JsonObject(List<Point> points) {
        this.points = points;
    }

    // Método para convertir el JSON a un objeto
    public static JsonObject fromJson(String jsonStr) {
        try {
            // Parseamos el JSON recibido como string
            JSONObject jsonObject = new JSONObject(jsonStr);
            JSONArray coordenadasArray = jsonObject.getJSONArray("coordenadas");
            List<Point> points = new ArrayList<>();

            // Iteramos sobre las coordenadas y las procesamos
            for (int i = 0; i < coordenadasArray.length(); i++) {
                JSONObject coord = coordenadasArray.getJSONObject(i);
                int x = coord.getInt("x");
                int y = coord.getInt("y");
                int w = coord.getInt("w");
                int h = coord.getInt("h");

                // Añadimos un nuevo punto con las coordenadas
                points.add(new Point(x, y, w, h));
            }

            return new JsonObject(points);
        } catch (JSONException e) {
            throw new RuntimeException("Error al parsear el JSON: " + e.getMessage());
        }
    }

    // Getter para la lista de puntos
    public List<Point> getPoints() {
        return points;
    }

}