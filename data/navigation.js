require([
    "geolocate", // geolocation simulator (https://github.com/2gis/mock-geolocation)
    "esri/widgets/Track",
    "esri/views/SceneView",
    "esri/Map",
    "esri/Graphic",
    "esri/layers/GraphicsLayer",
    "esri/Camera"
], function (geolocate, Track, SceneView, Map, Graphic, GraphicsLayer, Camera) {

    var map = new Map({
        basemap: "satellite",
        ground: "world-elevation"

    });

    class Diver {
        constructor(lng, lat, graphicsLayer) {
            this.down = false;
            this.color = "darkcyan"
            this.gl = graphicsLayer;
            this.lng = lng;
            this.lat = lat;
            this.point = {
                type: "point",
                longitude: this.lng,
                latitude: this.lat
            };

            this.marker = {
                type: "simple-marker",
                color: this.color, // orange
                outline: {
                    color: [255, 255, 255], // white
                    width: 1
                }
            };
            this.graphic = new Graphic({
                geometry: this.point,
                symbol: this.marker
            });
            this.gl.add(this.graphic);
        }
        setLocation(lng, lat){
            this.lng = lng;
            this.lat = lat;
            this.gl.remove(this.graphic);
            this.point = {
                type: "point",
                longitude: this.lng,
                latitude: this.lat
            };
            this.graphic = new Graphic({
                geometry: this.point,
                symbol: this.marker
            });
            this.gl.add(this.graphic);
        }

        setColor(color){
            this.gl.remove(this.graphic);
            this.color = color;
            this.marker = {
                type: "simple-marker",
                color: this.color, // orange
                outline: {
                    color: [255, 255, 255], // white
                    width: 1
                }
            };
            this.graphic = new Graphic({
                geometry: this.point,
                symbol: this.marker
            });
            this.gl.add(this.graphic);

        }
    }

    function httpGet(theUrl)
    {
        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open( "GET", theUrl, false ); // false for synchronous request
        xmlHttp.send( null );
        return xmlHttp.responseText;
    }

    let lat = -4.62409;
    let lng = 55.38619;
    let center = [lng, lat];
    let heading = 0.0;
    let count = 0;
    const zoom = 15;
    const dt = 50;

    let graphicsLayer = new GraphicsLayer();
    let diverA = new Diver( 55.38619,-4.62409, graphicsLayer);
    map.add(graphicsLayer);

    var view = new SceneView({
        container: "viewDiv",
        map: map,
        camera: {
            position: {
                // observation point
                x: lng,
                y: lat,
                z: 500, // altitude in meters
            },
            tilt: 0, // perspective in degrees
            heading: heading,
        }
    });

    setInterval(()=>{
        responseText = httpGet("/var/gps");
        let gpsData = JSON.parse(responseText);
        console.log(gpsData);
        if(gpsData.lng !==""){
            let newLng = parseFloat(gpsData.lng);
            let newLat = parseFloat(gpsData.lat);
            diverA.setLocation(newLng, newLat);
            view.goTo({
                position: {
                    x: newLng,
                    y: newLat,
                    z: 500, // altitude in meters
                },
                heading: heading,
                tilt: 0
            },)
                .catch(()=>{});
        }


    },2000)
});
