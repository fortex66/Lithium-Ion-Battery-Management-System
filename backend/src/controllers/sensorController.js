const { saveSensorData } = require("../models/sensorData");
const tcpServer = require("../services/tcpService");
const getConnection = require("../config/db");

async function handleSensorData(sensorData, io) {
  await saveSensorData(sensorData);
  io.emit("sensor_data", sensorData);
}

function handleUpdateRelayState(relayState) {
  tcpServer.sendRelayStateToRaspberryPi(relayState);
  console.log("Sent relay state to Raspberry Pi:", relayState);
}

// 설정한 핸들러를 tcpServer에 주입
tcpServer.onSensorDataReceived = (sensorData) => {
  const io = require("../server").io;
  handleSensorData(sensorData, io);
};

async function queryBatteryData(batteryNumber, startTime, endTime) {
  const connection = await getConnection();
  const query = `
    SELECT 
      voltage_${batteryNumber}, 
      current_${batteryNumber}, 
      soc_${batteryNumber}, 
      temperature_${batteryNumber}, 
      fan_pwm_${batteryNumber}, 
      resister_temp_${batteryNumber}, 
      duty_cycle${batteryNumber},
      resister_fan_pwm,
      timestamp
    FROM 
      sensor_readings 
    WHERE 
      timestamp BETWEEN ? AND ?
  `;

  const [rows] = await connection.execute(query, [startTime, endTime]);
  await connection.end();
  return rows;
}

module.exports = { handleSensorData, handleUpdateRelayState, queryBatteryData };
