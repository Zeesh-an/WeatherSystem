from flask import Flask, render_template
import plotly.express as px
import pandas as pd
from datetime import datetime, timedelta
from statsmodels.tsa.arima.model import ARIMA
from sklearn.metrics import mean_squared_error
from math import sqrt
import plotly.graph_objects as go

app = Flask(__name__)


def generate_plots(df, forecast):
    # Filter data for the last 12 hours
    last_12_hours = df[df['Time'] >= (datetime.now() - timedelta(hours=12))]

    
    # Create individual plots for each attribute
    temperature_plot = px.line(last_12_hours, x='Time', y='Temperature (C)', title='Temperature over the last 12 hours')
    humidity_plot = px.line(last_12_hours, x='Time', y='Humidity (%)', title='Humidity over the last 12 hours')
    feels_plot = px.line(last_12_hours, x='Time', y='Feels Like', title='Feels like Temperature over the last 12 hours')
    pressure_plot = px.line(last_12_hours, x='Time', y="Pressure", title='Pressure over the last 12 hours')
    combined_plot = px.line(last_12_hours, x='Time', y=['Temperature (C)','Humidity (%)','Feels Like',"Pressure"],
                            title='Sensor Data over the last 12 hours', labels={'value': 'Measurement'})

    if forecast is not None:
        forecast_times = pd.date_range(last_12_hours['Time'].max(), periods=len(forecast)+1, freq='60T')[0:]
        # forecasted_values = [last_12_hours["Temperature (C)"][-1]].extend(list(forecast["Predicted Temp"])) 
        forecasted_values = list(forecast["Predicted Temp"]) 
        forecasted_values.insert(0, last_12_hours["Temperature (C)"][-1])
        print(forecasted_values)
        # forecast_df = pd.DataFrame({'Time': forecast_times, 'Forecast': forecast["Predicted Temp"]})
        forecast_df = pd.DataFrame({'Time': forecast_times, 'Forecast': forecasted_values})
        # print(forecast_df)
        # forecast_df = pd.concat([new_row, forecast_df])
        forecast_plot = px.line(forecast_df, x='Time', y='Forecast', line_dash='Forecast', title='ARIMA Forecast')
        forecast_plot.update_traces(line=dict(color='red'))
        forecast_plot_html = forecast_plot.to_html(full_html=False)
        # print(forecast_df['Time'])
        temperature_plot.add_trace(go.Scatter(x=forecast_df['Time'], y=forecast_df['Forecast'], mode='lines', name='ARIMA Forecast', line=dict(color='grey', dash='dash')))
    else:
        forecast_plot_html = None

    # Convert the combined plot to HTML format
    combined_plot_html = combined_plot.to_html(full_html=False)
    # Convert plots to HTML format
    temperature_plot_html = temperature_plot.to_html(full_html=False)
    humidity_plot_html = humidity_plot.to_html(full_html=False)
    feels_plot_html = feels_plot.to_html(full_html=False)
    pressure_plot_html = pressure_plot.to_html(full_html=False)

    return temperature_plot_html, humidity_plot_html, feels_plot_html, pressure_plot_html,combined_plot_html, forecast_plot_html

@app.route('/')
def index():
    # Load the dummy data
    freq = '60T'
    df = pd.read_excel('data.xlsx')
    df['Timestamp'] = pd.to_datetime(df['Time'])
    df.set_index('Timestamp', inplace=True)
    df = df.asfreq(freq)
    # Choose the target variable for prediction (e.g., 'Temperature')
    target_variable = 'Temperature (C)'

    # Fit ARIMA model
    order = (10, 1, 2)  # Example order parameters (p, d, q)
    model = ARIMA(df[target_variable], order=order)
    model_fit = model.fit()

    # Make predictions for the next three hours (12 time intervals if your data is in 15-minute intervals)
    forecast_steps = 6
    forecast = model_fit.forecast(steps=forecast_steps, typ='levels')
    forecast = pd.DataFrame(forecast)
    forecast.columns = ["Predicted Temp"]
    
    # Generate plots
    temperature_plot_html, humidity_plot_html, feels_plot_html, pressure_plot_html, combined_plot_html, forecast_plot_html = generate_plots(df, forecast)

    # Render the HTML template with the plots
    return render_template('index.html', temperature_plot=temperature_plot_html,
                           humidity_plot=humidity_plot_html, feels_plot=feels_plot_html, pressure_plot=pressure_plot_html,combined_plot = combined_plot_html,
                           forecast_plot = forecast_plot_html)

if __name__ == '__main__':
    app.run(debug=True)
