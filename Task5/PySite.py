from flask import Flask, request, render_template, jsonify
import requests
import matplotlib.pyplot as plt
from io import BytesIO
import base64
from datetime import datetime

app = Flask(__name__)

def fetch_data(params):
    url = "http://localhost:8080/data"
    try:
        response = requests.get(url, params=params)
        response.raise_for_status()

        if "Temperature data:" in response.text or "Latest record:" in response.text:
            data = []
            lines = response.text.splitlines()
            if len(lines) > 1:
                for line in lines:
                    if line.startswith("Date:"):
                        parts = line.split(", ")
                        date_str = parts[0].split(": ")[1]
                        temperature = float(parts[1].split(": ")[1])
                        try:
                            data.append((datetime.strptime(date_str, "%Y-%m-%d %H:%M:%S"), temperature))
                        except ValueError as e:
                            print(f"Error parsing date: {e}, Line: {line}")
                            return None
            else:
                print(f"Empty data after 'Temperature data:' for {params}")
                return None
            return data
        else:
            print(f"Unexpected response format for {params}: {response.text}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"Error fetching data: {e}")
        return None

def create_plot(data, title):
    if not data:
        print(f"No data to create plot for: {title}")
        return None

    try:
        dates, temperatures = zip(*data)

        plt.figure(figsize=(10, 6))
        plt.plot(dates, temperatures, marker="o", linestyle="-", label=title)
        plt.title(title)
        plt.xlabel("Date")
        plt.ylabel("Temperature (°C)")
        plt.grid(True)
        plt.xticks(rotation=45, ha='right')
        plt.tight_layout()
        plt.legend()

        img_buf = BytesIO()
        plt.savefig(img_buf, format="png")
        img_buf.seek(0)
        img_base64 = base64.b64encode(img_buf.read()).decode('utf-8')
        plt.close()
        return img_base64
    except Exception as e:
        print(f"Error creating plot: {e}")
        return None


@app.route('/')
def index():
    return render_template('index.html')

@app.route('/get_current_temperature', methods=['GET'])
def get_current_temperature():
    params = {"last": "true", "table": "data_current"}
    data = fetch_data(params)
    if data:
        return jsonify({
            "date": data[0][0].strftime("%Y-%m-%d %H:%M:%S"),
            "temperature": data[0][1]
        })
    return jsonify({"error": "No data found"}), 404

@app.route('/get_graphs', methods=['GET'])
def get_graphs():
    start_date = request.args.get("start_date", "1970-01-01 00:00:00")
    end_date = request.args.get("end_date", "2026-01-01 00:00:00")
    tables = ["data_current", "data_hour", "data_day"]
    graphs = {}
    for table in tables:
        params = {"start": start_date, "end": end_date, "table": table}
        data = fetch_data(params)
        if data:
            graph_data = create_plot(data, f"Data from {table}")
            if graph_data:
                graphs[f"{table}_graph"] = graph_data
            else:
                graphs[f"{table}_graph"] = "No graph data available."
        else:
            graphs[f"{table}_graph"] = "No data available for this table and time period."

    return jsonify(graphs)

if __name__ == '__main__':
    app.run(debug=True)