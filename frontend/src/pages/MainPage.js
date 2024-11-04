import React, { useEffect, useState } from "react";
import { io } from "socket.io-client";
import BatterySystemDiagram from "../components/systemDiagram";

const MainPage = () => {
  const [data, setData] = useState({
    temperature: [0, 0, 0],
    resisterTemp: [0, 0, 0],
    voltage: [0, 0, 0],
    current: [0, 0, 0],
    fanSpeed: [0, 0, 0],
    batteryPercentage: [0, 0, 0],
    chargeMode: [0, 0, 0],
    relayState: [0, 0, 0],
    resisterFanPwm: 0,
  });

  useEffect(() => {
    const socket = io("http://192.168.0.152:3000");
    socket.on("sensor_data", (receivedData) => {
      console.log(receivedData);
      const {
        voltage_1,
        current_1,
        soc_1,
        temperature_1,
        charge_mode_1,
        fan_pwm_1,
        relay_state_1,
        resister_temp_1,
        voltage_2,
        current_2,
        soc_2,
        temperature_2,
        charge_mode_2,
        fan_pwm_2,
        relay_state_2,
        resister_temp_2,
        voltage_3,
        current_3,
        soc_3,
        temperature_3,
        charge_mode_3,
        fan_pwm_3,
        relay_state_3,
        resister_temp_3,
        resister_fan_pwm,
      } = receivedData;

      setData({
        temperature: [temperature_1, temperature_2, temperature_3],
        resisterTemp: [resister_temp_1, resister_temp_2, resister_temp_3],
        voltage: [voltage_1, voltage_2, voltage_3],
        current: [current_1, current_2, current_3],
        fanSpeed: [fan_pwm_1, fan_pwm_2, fan_pwm_3],
        batteryPercentage: [soc_1, soc_2, soc_3],
        chargeMode: [charge_mode_1, charge_mode_2, charge_mode_3],
        relayState: [relay_state_1, relay_state_2, relay_state_3],
        resisterFanPwm: resister_fan_pwm,
      });
    });

    return () => {
      socket.disconnect();
    };
  }, []);

  return <BatterySystemDiagram data={data} />;
};

export default MainPage;
