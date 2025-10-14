from flask import Flask, request
import os
app = Flask(__name__)

save_dir = "dataset"
os.makedirs(save_dir, exist_ok=True)
counter = 0

@app.route("/upload", methods=["POST"])
def upload():
    global counter
    data = request.data
    filename = f"{save_dir}/img_{counter:04d}.pgm"
    with open(filename, "wb") as f:
        f.write(data)
    counter += 1
    print(f"Imagen guardada: {filename}")
    return "OK", 200

app.run(host="0.0.0.0", port=5000)
