const getConnection = require("../config/db");

async function saveSensorData(sensorData) {
  const connection = await getConnection();
  const query = `
    INSERT INTO sensor_readings (
      voltage_1, current_1, soc_1, temperature_1, charge_mode_1, relay_state_1, fan_pwm_1, resister_temp_1, duty_cycle1,
      voltage_2, current_2, soc_2, temperature_2, charge_mode_2, relay_state_2, fan_pwm_2, resister_temp_2, duty_cycle2,
      voltage_3, current_3, soc_3, temperature_3, charge_mode_3, relay_state_3, fan_pwm_3, resister_temp_3, duty_cycle3,
      resister_fan_pwm
    ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
  `;

  const values = [
    sensorData.voltage_1,
    sensorData.current_1,
    sensorData.soc_1,
    sensorData.temperature_1,
    sensorData.charge_mode_1,
    sensorData.relay_state_1,
    sensorData.fan_pwm_1,
    sensorData.resister_temp_1,
    sensorData.duty_cycle1,
    sensorData.voltage_2,
    sensorData.current_2,
    sensorData.soc_2,
    sensorData.temperature_2,
    sensorData.charge_mode_2,
    sensorData.relay_state_2,
    sensorData.fan_pwm_2,
    sensorData.resister_temp_2,
    sensorData.duty_cycle2,
    sensorData.voltage_3,
    sensorData.current_3,
    sensorData.soc_3,
    sensorData.temperature_3,
    sensorData.charge_mode_3,
    sensorData.relay_state_3,
    sensorData.fan_pwm_3,
    sensorData.resister_temp_3,
    sensorData.duty_cycle3,
    sensorData.resister_fan_pwm,
  ];

  await connection.execute(query, values);
  await connection.end();
}

module.exports = { saveSensorData };
