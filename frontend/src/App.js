import React, { useEffect, useState } from "react";
import { BrowserRouter as Router, Route, Routes } from "react-router-dom";
import { io } from "socket.io-client";
import NavigationBar from "./components/NavigationBar";
import MainPage from "./pages/MainPage";
import ChartPage from "./pages/ChartPage";
import DataPage from "./pages/DataPage";
import SettingPage from "./pages/SettingPage";
import styled from "styled-components";

const socket = io("http://192.168.0.152:3000");

function App() {
  const [data, setData] = useState({
    temperatures: [[], [], []],
    resisterTemps: [[], [], []],
    voltages: [[], [], []],
    currents: [[], [], []],
    duty_cycles: [[], [], []],
    times: [],
    relayState: [0, 0, 0],
  });

  useEffect(() => {
    socket.on("sensor_data", (receivedData) => {
      const {
        voltage_1,
        current_1,
        soc_1,
        duty_cycle1,
        temperature_1,
        charge_mode_1,
        fan_pwm_1,
        relay_state_1,
        resister_temp_1,
        voltage_2,
        current_2,
        duty_cycle2,
        soc_2,
        temperature_2,
        charge_mode_2,
        fan_pwm_2,
        relay_state_2,
        resister_temp_2,
        voltage_3,
        current_3,
        duty_cycle3,
        soc_3,
        temperature_3,
        charge_mode_3,
        fan_pwm_3,
        relay_state_3,
        resister_temp_3,
      } = receivedData;

      const now = Date.now() / 1000;

      setData((prevData) => ({
        ...prevData,
        times: [...prevData.times, now].slice(-60),
        temperatures: [
          [...prevData.temperatures[0], temperature_1].slice(-60),
          [...prevData.temperatures[1], temperature_2].slice(-60),
          [...prevData.temperatures[2], temperature_3].slice(-60),
        ],
        resisterTemps: [
          [...prevData.resisterTemps[0], resister_temp_1].slice(-60),
          [...prevData.resisterTemps[1], resister_temp_2].slice(-60),
          [...prevData.resisterTemps[2], resister_temp_3].slice(-60),
        ],
        voltages: [
          [...prevData.voltages[0], voltage_1].slice(-60),
          [...prevData.voltages[1], voltage_2].slice(-60),
          [...prevData.voltages[2], voltage_3].slice(-60),
        ],
        currents: [
          [...prevData.currents[0], (current_1 / 1000).toFixed(2)].slice(-60),
          [...prevData.currents[1], (current_2 / 1000).toFixed(2)].slice(-60),
          [...prevData.currents[2], (current_3 / 1000).toFixed(2)].slice(-60),
        ],
        duty_cycles: [
          [...prevData.duty_cycles[0], duty_cycle1].slice(-60),
          [...prevData.duty_cycles[1], duty_cycle2].slice(-60),
          [...prevData.duty_cycles[2], duty_cycle3].slice(-60),
        ],
        relayState: [relay_state_1, relay_state_2, relay_state_3],
      }));
    });

    return () => {
      socket.disconnect();
    };
  }, []);

  return (
    <Router>
      <div>
        <NavigationBar />
        <Content>
          <Routes>
            <Route path="/" element={<MainPage />} />
            <Route path="/chart" element={<ChartPage data={data} />} />
            <Route path="/data" element={<DataPage />} />
            <Route path="/setting" element={<SettingPage />} />
          </Routes>
        </Content>
      </div>
    </Router>
  );
}

export default App;

const Content = styled.div`
  padding: 0px;
`;
