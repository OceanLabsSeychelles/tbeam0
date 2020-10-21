double deg2rad(double deg) {
  return (deg * PI / 180);
}
double rad2deg(double rad) {
  return (rad * 180 / PI);
}
double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(lat1d);
  lon1r = deg2rad(lon1d);
  lat2r = deg2rad(lat2d);
  lon2r = deg2rad(lon2d);
  u = sin((lat2r - lat1r)/2);
  v = sin((lon2r - lon1r)/2);
  return (2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v)))*1000.0;
}

double getBearing(double lat,double lon,double lat2,double lon2){

    double teta1 = deg2rad(lat);
    double teta2 = deg2rad(lat2);
    double delta1 = deg2rad(lat2-lat);
    double delta2 = deg2rad(lon2-lon);

    //==================Heading Formula Calculation================//

    double y = sin(delta2) * cos(teta2);
    double x = cos(teta1)*sin(teta2) - sin(teta1)*cos(teta2)*cos(delta2);
    double brng = atan2(y,x);
    brng = rad2deg(brng);// radians to degrees
    brng = ( ((int)brng + 360) % 360 ); 

    Serial.print("Heading GPS: ");
    Serial.println(brng);

    return brng;

  }