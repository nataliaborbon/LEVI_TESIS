#include "html_content.h"

const char *aplicacionHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tesis - Eye Tracking (Template Matching)</title>
    <style>
        body { background-color: #222; color: white; text-align: center; font-family: sans-serif; }
        .paneles { display: flex; flex-wrap: wrap; justify-content: center; gap: 30px; margin-top: 15px; }
        .panel { background: #333; padding: 15px; border-radius: 8px; display: flex; flex-direction: column; align-items: center; }
        .panel h3 { margin: 0 0 10px 0; font-size: 16px; }
        
        canvas { border: 2px solid #555; border-radius: 4px; background: #000; image-rendering: pixelated; }
        /* Agrandamos un poco el área de visión para la gorra (128x128 píxeles internos, mostrados al doble) */
        #canvasOriginal { border-color: #f1c40f; width: 256px; height: 256px; cursor: crosshair; }
        #canvasOutput { border-color: #2ecc71; width: 256px; height: 256px; } 
        #canvasTemplate { border-color: #3498db; width: 96px; height: 96px; } 
        
        .oculto-activo { position: absolute; top: -9999px; left: -9999px; visibility: hidden; }

        #estado { font-weight: bold; color: #f39c12; margin: 5px; font-size: 18px;}
        .instrucciones { margin: 15px; padding: 15px; background: #2c3e50; border-radius: 8px; display: inline-block; border-left: 4px solid #f1c40f; text-align: left; }
        .controles { margin-top: 10px; }
        input[type=range] { width: 200px; cursor: pointer; vertical-align: middle; }
    </style>
</head>
<body>
    <h2>Dashboard - Eye Tracking (Rastreo Fotográfico)</h2>
    <p id="estado">Cargando OpenCV.js... (Paciencia)</p>

    <div class="instrucciones">
        <b>🎯 CÓMO CALIBRAR:</b><br>
        1. Ponete la gorra y mirá al frente.<br>
        2. Hacé <b>clic exactamente sobre tu pupila</b> en el recuadro amarillo (Ojo Original).<br>
        <div class="controles">
            <label>Tamaño del recuadro de captura: <span id="val_tam">24</span>px</label>
            <input type="range" id="slider_tam" min="10" max="50" value="24">
        </div>
    </div>

    <img id="streamS3" class="oculto-activo" src="http://192.168.4.50/stream" crossorigin="anonymous" width="320" height="240">
    <canvas id="hiddenCanvas" class="oculto-activo" width="320" height="240"></canvas>

    <div class="paneles">
        <div class="panel">
            <h3 style="color: #f1c40f;">1. Ojo Original (¡Clickeá acá!)</h3>
            <canvas id="canvasOriginal" width="128" height="128"></canvas>
        </div>

        <div class="panel">
            <h3 style="color: #2ecc71;">2. Tracking Activo</h3>
            <canvas id="canvasOutput" width="128" height="128"></canvas>
        </div>

        <div class="panel">
            <h3 style="color: #3498db;">3. Molde (Tu Pupila)</h3>
            <canvas id="canvasTemplate" width="24" height="24"></canvas>
        </div>
    </div>

    <script async src="opencv.js" onload="onOpenCvReady();" type="text/javascript"></script>

    <script>
        const imgStream = document.getElementById('streamS3');
        const hiddenCanvas = document.getElementById('hiddenCanvas');
        const ctx = hiddenCanvas.getContext('2d', { willReadFrequently: true });
        
        const canvasOriginal = document.getElementById('canvasOriginal');
        const canvasTemplate = document.getElementById('canvasTemplate');
        const estado = document.getElementById('estado');
        
        const sliderTam = document.getElementById('slider_tam');
        const valTam = document.getElementById('val_tam');

        // Ampliamos el área de captura para darte más libertad de movimiento con la gorra
        const TAMAÑO_CORTE = 128; 
        const SUAVIZADO = 0.6;
        
        let TAM_MOLDE = 24;
        sliderTam.oninput = function() { 
            TAM_MOLDE = parseInt(this.value); 
            valTam.innerText = this.value; 
            // Ajustamos el tamaño visual del canvas del molde
            canvasTemplate.width = TAM_MOLDE;
            canvasTemplate.height = TAM_MOLDE;
        };

        let ultimo_x = TAMAÑO_CORTE / 2, ultimo_y = TAMAÑO_CORTE / 2;
        let frames_perdidos = 0;

        let src, dst, gray, moldeMat = null;

        function onOpenCvReady() {
            cv['onRuntimeInitialized'] = () => {
                estado.innerText = "OpenCV Listo. Conectando cámara...";
                iniciarCamaraS3();
            };
        }

        function iniciarCamaraS3() {
            src = new cv.Mat(240, 320, cv.CV_8UC4);
            dst = new cv.Mat(TAMAÑO_CORTE, TAMAÑO_CORTE, cv.CV_8UC4);
            gray = new cv.Mat();
            
            estado.innerText = "¡Cámara Activa! Hacé clic en tu pupila.";
            estado.style.color = "#2ecc71";
            
            requestAnimationFrame(procesarVideo);
        }

        // --- EL EVENTO DE DISPARO (CAPTURAR LA PUPILA) ---
        canvasOriginal.addEventListener('mousedown', function(event) {
            if (gray.empty()) return;

            // Calculamos en qué coordenada X,Y del canvas hizo clic el usuario
            const rectHTML = canvasOriginal.getBoundingClientRect();
            const scaleX = canvasOriginal.width / rectHTML.width;
            const scaleY = canvasOriginal.height / rectHTML.height;
            
            let clickX = (event.clientX - rectHTML.left) * scaleX;
            let clickY = (event.clientY - rectHTML.top) * scaleY;

            // Creamos un cuadrado centrado en el clic
            let x1 = Math.round(clickX - (TAM_MOLDE / 2));
            let y1 = Math.round(clickY - (TAM_MOLDE / 2));

            // Protecciones para no salirnos de los bordes de la imagen
            if (x1 < 0) x1 = 0;
            if (y1 < 0) y1 = 0;
            if (x1 + TAM_MOLDE > TAMAÑO_CORTE) x1 = TAMAÑO_CORTE - TAM_MOLDE;
            if (y1 + TAM_MOLDE > TAMAÑO_CORTE) y1 = TAMAÑO_CORTE - TAM_MOLDE;

            // Recortamos ese pedacito de la imagen en blanco y negro (El Molde)
            let rectMolde = new cv.Rect(x1, y1, TAM_MOLDE, TAM_MOLDE);
            
            if (moldeMat != null) moldeMat.delete(); // Borramos el molde viejo si existía
            moldeMat = gray.roi(rectMolde).clone();  // Guardamos el nuevo molde clonado

            // Lo mostramos en el Panel 3 para que el usuario vea qué capturó
            cv.imshow('canvasTemplate', moldeMat);
            
            estado.innerText = "¡Pupila fijada! Tracking automático activado.";
            estado.style.color = "#3498db";
        });

        function procesarVideo() {
            try {
                // 1. DIBUJAR Y EXTRAER STREAM
                ctx.drawImage(imgStream, 0, 0, 320, 240);
                let imgData = ctx.getImageData(0, 0, 320, 240);
                src.data.set(imgData.data);
                cv.flip(src, src, 1);

                // 2. RECORTAR ROI FIJO (El área donde se mueve el ojo)
                let cx = Math.floor(src.cols / 2);
                let cy = Math.floor(src.rows / 2);
                let rect = new cv.Rect(cx - (TAMAÑO_CORTE/2), cy - (TAMAÑO_CORTE/2), TAMAÑO_CORTE, TAMAÑO_CORTE);
                let roi_recorte = src.roi(rect);
                
                roi_recorte.copyTo(dst);

                // Convertimos el frame actual a grises
                cv.cvtColor(roi_recorte, gray, cv.COLOR_RGBA2GRAY, 0);

                // 3. LA MAGIA: TEMPLATE MATCHING
                if (moldeMat !== null && !moldeMat.empty()) {
                    let resultado = new cv.Mat();
                    
                    // Compara el molde capturado contra todo el recuadro actual buscando la mejor coincidencia fotográfica
                    cv.matchTemplate(gray, moldeMat, resultado, cv.TM_CCOEFF_NORMED);
                    
                    // Busca el punto con la puntuación de similitud más alta
                    let loc = cv.minMaxLoc(resultado);
                    let maxPoint = loc.maxLoc; // En el método CCOEFF_NORMED, el más alto es el mejor
                    
                    // La coordenada que devuelve es la esquina superior izquierda del molde.
                    // Le sumamos la mitad para encontrar el centro exacto de la pupila.
                    let centro_x_nuevo = maxPoint.x + (TAM_MOLDE / 2);
                    let centro_y_nuevo = maxPoint.y + (TAM_MOLDE / 2);

                    // Suavizado de movimiento
                    ultimo_x = ultimo_x + SUAVIZADO * (centro_x_nuevo - ultimo_x);
                    ultimo_y = ultimo_y + SUAVIZADO * (centro_y_nuevo - ultimo_y);

                    // Dibuja el recuadro verde rastreando la pupila
                    let pt1 = new cv.Point(Math.round(ultimo_x - TAM_MOLDE/2), Math.round(ultimo_y - TAM_MOLDE/2));
                    let pt2 = new cv.Point(Math.round(ultimo_x + TAM_MOLDE/2), Math.round(ultimo_y + TAM_MOLDE/2));
                    cv.rectangle(dst, pt1, pt2, new cv.Scalar(0, 255, 0, 255), 2);
                    
                    // Dibuja el centro rojo
                    cv.circle(dst, new cv.Point(Math.round(ultimo_x), Math.round(ultimo_y)), 2, new cv.Scalar(255, 0, 0, 255), -1);

                    resultado.delete();
                }

                // 4. MOSTRAR PANELES
                cv.imshow('canvasOriginal', roi_recorte); 
                cv.imshow('canvasOutput', dst);           
                
                roi_recorte.delete();
                requestAnimationFrame(procesarVideo);

            } catch (err) {
                console.error(err);
                estado.innerText = "🚨 ERROR: " + err.message;
                estado.style.color = "#e74c3c";
            }
        }
    </script>
</body>
</html>
)rawliteral";