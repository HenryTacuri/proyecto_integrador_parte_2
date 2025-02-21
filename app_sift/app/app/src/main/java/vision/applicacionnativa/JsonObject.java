package vision.applicacionnativa;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

class Point {
    double[] pt;
    double size;
    double angle;
    double response;
    int octave;
    int class_id;

    // Constructor
    public Point(double[] pt, double size, double angle, double response, int octave, int class_id) {
        this.pt = pt;
        this.size = size;
        this.angle = angle;
        this.response = response;
        this.octave = octave;
        this.class_id = class_id;
    }
}

public class JsonObject {
    List<Point> points; // Lista de puntos que corresponde a tu array de objetos

    // Constructor
    public JsonObject(List<Point> points) {
        this.points = points;
    }

    // Método para convertir el JSON a un objeto
    public static JsonObject fromJson(String jsonStr) {
        try {
            JSONArray jsonArray = new JSONArray(jsonStr);
            List<Point> points = new ArrayList<>();

            for (int i = 0; i < jsonArray.length(); i++) {
                JSONObject jsonObject = jsonArray.getJSONObject(i);
                JSONArray ptArray = jsonObject.getJSONArray("pt");
                double[] pt = new double[ptArray.length()];
                for (int j = 0; j < ptArray.length(); j++) {
                    pt[j] = ptArray.getDouble(j);
                }
                double size = jsonObject.getDouble("size");
                double angle = jsonObject.getDouble("angle");
                double response = jsonObject.getDouble("response");
                int octave = jsonObject.getInt("octave");
                int classId = jsonObject.getInt("class_id");

                points.add(new Point(pt, size, angle, response, octave, classId));
            }

            return new JsonObject(points);
        } catch (JSONException e) {
            throw new RuntimeException(e);
        }

    }
}