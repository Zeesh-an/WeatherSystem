from flask import Flask, render_template
import plotly.express as px
import pandas as pd
from datetime import datetime, timedelta

app = Flask(__name__)

def generate_plots(df):
    # Filter data for the last 12 hours
    last_12_hours = df[df['Time'] >= (datetime.now() - timedelta(hours=12))]

    # Create individual plots for each attribute
    temperature_plot = px.line(last_12_hours, x='Time', y='Temperature (C)', title='Temperature over the last 12 hours')
    humidity_plot = px.line(last_12_hours, x='Time', y='Humidity (%)', title='Humidity over the last 12 hours')
    co_plot = px.line(last_12_hours, x='Time', y='CO (ppm)', title='CO over the last 12 hours')
    combined_plot = px.line(last_12_hours, x='Time', y=['Temperature (C)', 'Humidity (%)', 'CO (ppm)'],
                            title='Sensor Data over the last 12 hours', labels={'value': 'Measurement'})

    # Convert the combined plot to HTML format
    combined_plot_html = combined_plot.to_html(full_html=False)
    # Convert plots to HTML format
    temperature_plot_html = temperature_plot.to_html(full_html=False)
    humidity_plot_html = humidity_plot.to_html(full_html=False)
    co_plot_html = co_plot.to_html(full_html=False)

    return temperature_plot_html, humidity_plot_html, co_plot_html, combined_plot_html

@app.route('/')
def index():
    # Load the dummy data
    df = pd.read_excel('/home/zeeshan/IOT_Project/data.xlsx')

    # Generate plots
    temperature_plot_html, humidity_plot_html, co_plot_html, combined_plot_html = generate_plots(df)

    # Render the HTML template with the plots
    return render_template('index.html', temperature_plot=temperature_plot_html,
                           humidity_plot=humidity_plot_html, co_plot=co_plot_html, combined_plot = combined_plot_html)

if __name__ == '__main__':
    app.run(debug=True)
