import QtQuick
import QtLocation
import QtPositioning
import QtQml.Models // <--- ÚJ IMPORT A ListModel-hez!

Item {
    width: 800
    height: 600

    Plugin {
        id: mapPlugin
        name: "osm"
        // Használjuk a CyclOSM-et, azon jól látszanak a domborzatok
        PluginParameter { name: "osm.useragent"; value: "TuraTervezoApp/1.0" }
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://a.tile-cyclosm.openstreetmap.fr/cyclosm/" }
        PluginParameter { name: "osm.mapping.custom.mapcopyright"; value: "CyclOSM / OpenStreetMap" }
        PluginParameter { name: "osm.mapping.custom.format"; value: "png" }
        PluginParameter { name: "osm.mapping.cache.directory"; value: "cache_cyclosm_gradient" }
    }

    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(47.53, 18.98)
        zoomLevel: 13

        Component.onCompleted: {
            for (var i = 0; i < map.supportedMapTypes.length; ++i) {
                if (map.supportedMapTypes[i].name === "Custom URL Map") map.activeMapType = map.supportedMapTypes[i]
            }
        }

        DragHandler { id: drag; target: null; onTranslationChanged: (delta) => map.pan(-delta.x, -delta.y) }
        WheelHandler { id: wheel; acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad; rotationScale: 1/120; property: "zoomLevel" }

        // --- 1. ALAPRÉTEG: Fehér keret ---
        // Ez adja a kontúrt, hogy bármilyen térképen látszódjon
        MapPolyline {
            id: routeOutline
            line.width: 10 // Vastag keret
            line.color: "white"
            opacity: 0.7
        }

        // --- 2. SZÍNES MEREDEKSÉG RÉTEG (Hőtérkép) ---
        // Ez egy lista, ami sok kis színes vonalat tárol
        ListModel { id: segmentModel }

        MapItemView {
            model: segmentModel
            // A delegált mondja meg, hogyan rajzoljunk ki egy elemet a listából
            delegate: MapPolyline {
                line.width: 6 // Kicsit vékonyabb, mint a keret
                // A színt és a pontokat a listából (model) vesszük
                line.color: model.segmentColor
                path: [
                    QtPositioning.coordinate(model.lat1, model.lon1),
                    QtPositioning.coordinate(model.lat2, model.lon2)
                ]
            }
        }
        // ---------------------------------------------

        // Markerek
        MapQuickItem {
            id: startMarker; anchorPoint.x: sourceItem.width/2; anchorPoint.y: sourceItem.height; visible: false
            sourceItem: Rectangle { width: 20; height: 20; radius: 10; color: "lime"; border.color: "black"; border.width: 2 }
        }
        MapQuickItem {
            id: endMarker; anchorPoint.x: sourceItem.width/2; anchorPoint.y: sourceItem.height; visible: false
            sourceItem: Rectangle { width: 20; height: 20; radius: 10; color: "red"; border.color: "black"; border.width: 2 }
        }
        // --- STATISZTIKA DOBOZ (LEGEND) ---
        Rectangle {
            id: statsBox
            width: 160
            height: 110
            color: "#cc000000" // Félig átlátszó fekete (CC = 80% alpha)
            radius: 10
            visible: false // Alapból nem látszik, csak ha van adat

            // Jobb felső sarokba tesszük, kis margóval
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 20

            Column {
                anchors.centerIn: parent
                spacing: 5

                // Lejtő (Kék)
                Row {
                    spacing: 10
                    Rectangle { width: 15; height: 15; radius: 7.5; color: "#2196F3" }
                    Text { id: txtDown; text: "Lejtő: 0%"; color: "white"; font.bold: true }
                }
                // Könnyű (Zöld)
                Row {
                    spacing: 10
                    Rectangle { width: 15; height: 15; radius: 7.5; color: "#4CAF50" }
                    Text { id: txtFlat; text: "Könnyű: 0%"; color: "white"; font.bold: true }
                }
                // Közepes (Sárga)
                Row {
                    spacing: 10
                    Rectangle { width: 15; height: 15; radius: 7.5; color: "#FFC107" }
                    Text { id: txtMed; text: "Közepes: 0%"; color: "white"; font.bold: true }
                }
                // Meredek (Piros)
                Row {
                    spacing: 10
                    Rectangle { width: 15; height: 15; radius: 7.5; color: "#F44336" }
                    Text { id: txtSteep; text: "Meredek: 0%"; color: "white"; font.bold: true }
                }
            }
        }
    }

    // színes szakasz hozzáadása
    function addColoredSegment(lat1, lon1, lat2, lon2, colorStr) {
        // Hozzáadjuk a koordinátákat és a színt a listához
        segmentModel.append({
            "lat1": lat1, "lon1": lon1,
            "lat2": lat2, "lon2": lon2,
            "segmentColor": colorStr
        });
        // A fehér kerethez is hozzáadjuk a pontot (csak az elsőt, hogy folytonos legyen)
        routeOutline.addCoordinate(QtPositioning.coordinate(lat1, lon1));
    }

    // útvonal törlő
    function clearRoute() {
        routeOutline.path = []
        segmentModel.clear() // Töröljük a színes listát is
        startMarker.visible = false
        endMarker.visible = false
    }

    // markerek
    function setMarkers(startLat, startLon, endLat, endLon) {
        startMarker.coordinate = QtPositioning.coordinate(startLat, startLon)
        startMarker.visible = true
        endMarker.coordinate = QtPositioning.coordinate(endLat, endLon)
        endMarker.visible = true
    }

    // zoom
    function zoomToRect(minLat, minLon, maxLat, maxLon) {
        var latSpan = maxLat - minLat;
        var lonSpan = maxLon - minLon;
        var bufferRatio = 0.2;
        map.visibleRegion = QtPositioning.rectangle(
            QtPositioning.coordinate(maxLat + latSpan * bufferRatio, minLon - lonSpan * bufferRatio),
            QtPositioning.coordinate(minLat - latSpan * bufferRatio, maxLon + lonSpan * bufferRatio)
        )
    }

    // ez frissíti a százalékokat
        function updateStats(pctDown, pctFlat, pctMed, pctSteep) {
            txtDown.text  = "Lejtő: " + Math.round(pctDown) + "%"
            txtFlat.text  = "Könnyű: " + Math.round(pctFlat) + "%"
            txtMed.text   = "Közepes: " + Math.round(pctMed) + "%"
            txtSteep.text = "Meredek: " + Math.round(pctSteep) + "%"
            statsBox.visible = true
        }
}
