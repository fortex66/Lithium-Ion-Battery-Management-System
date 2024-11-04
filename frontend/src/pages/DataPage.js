import React, { useState } from "react";
import axios from "axios";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";
import styled from "styled-components";

const DataPage = () => {
  const [batteryNumber, setBatteryNumber] = useState("1");
  const [startTime, setStartTime] = useState("");
  const [endTime, setEndTime] = useState("");
  const [results, setResults] = useState(null);

  const handleSubmit = async (event) => {
    event.preventDefault();

    try {
      const response = await axios.post("http://192.168.0.152:3000/api/query", {
        batteryNumber,
        startTime,
        endTime,
      });

      let filteredData = response.data;

      // 데이터 필터링
      const start = new Date(startTime).getTime();
      const end = new Date(endTime).getTime();
      const duration = (end - start) / 1000; // duration in seconds

      if (duration > 3600) {
        // 1시간 이상일 때 절반만 가져오기
        filteredData = filterData(response.data, 4); // 1/4 데이터 가져오기
      } else if (duration > 1800) {
        // 30분 이상일 때 절반만 가져오기
        filteredData = filterData(response.data, 2); // 1/2 데이터 가져오기
      }

      setResults(filteredData.map((entry) => ({ ...entry, batteryNumber })));
    } catch (error) {
      console.error("Error querying data:", error);
    }
  };

  // 데이터 필터링 함수
  const filterData = (data, factor) => {
    return data.filter((_, index) => index % factor === 0);
  };

  const parseData = (data, batteryNumber) => {
    return data.map((entry) => {
      const date = new Date(entry.timestamp);
      const kstDate = new Date(date.getTime() + 9 * 60 * 60 * 1000); // KST로 변환 (UTC+9)
      return {
        timestamp: kstDate.toISOString().slice(0, 19).replace("T", " "),
        Volt: Number(entry[`voltage_${batteryNumber}`]).toFixed(2),
        Current: (entry[`current_${batteryNumber}`] / 1000).toFixed(2), // mA to A
        Charge_Temp: Number(entry[`temperature_${batteryNumber}`]).toFixed(2),
        SoC: entry[`soc_${batteryNumber}`],
        Charge_Fan_PWM: entry[`fan_pwm_${batteryNumber}`],
        Discharge_Temp: Number(entry[`resister_temp_${batteryNumber}`]).toFixed(
          2
        ),
        Charge_PWM: entry[`duty_cycle${batteryNumber}`],
        Discharge_Fan_PWM: entry[`resister_fan_pwm`],
      };
    });
  };

  return (
    <Container>
      <Title>Battery Data Query</Title>
      <Form onSubmit={handleSubmit}>
        <label>
          Battery Number:
          <select
            value={batteryNumber}
            onChange={(e) => setBatteryNumber(e.target.value)}
          >
            <option value="1">1</option>
            <option value="2">2</option>
            <option value="3">3</option>
          </select>
        </label>
        <label>
          Start Time:
          <input
            type="datetime-local"
            value={startTime}
            onChange={(e) => setStartTime(e.target.value)}
          />
        </label>
        <label>
          End Time:
          <input
            type="datetime-local"
            value={endTime}
            onChange={(e) => setEndTime(e.target.value)}
          />
        </label>
        <button type="submit">조회</button>
      </Form>
      {results && (
        <DataContainer>
          <ResponsiveContainer width="100%" height={550}>
            <LineChart
              data={parseData(results, batteryNumber)}
              margin={{ top: 5, right: 10, left: 10, bottom: 5 }}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis dataKey="timestamp" />
              <YAxis
                yAxisId="voltage"
                domain={[2.8, 4.2]}
                width={40}
                stroke="#ff0000"
              />
              <YAxis
                yAxisId="current"
                domain={[-4, 4]}
                width={30}
                stroke="green"
              />
              {/* <YAxis yAxisId="soc" domain={[0, 100]} width={25} stroke="blue" /> */}
              {/* <YAxis
                yAxisId="temperature"
                orientation="right"
                domain={[0, 50]}
                width={30}
                stroke="green"
              /> */}
              {/* <YAxis
                yAxisId="resister_temp"
                orientation="right"
                domain={[0, 50]}
                width={30}
                stroke="orange"
              /> */}

              <YAxis
                yAxisId="duty_cycle"
                orientation="right"
                domain={[0, 100]}
                width={40}
                stroke="#ff33cc"
              />

              {/* <YAxis
                yAxisId="fan_pwm"
                orientation="right"
                domain={[0, 100]}
                width={40}
                stroke="rgb(255 111 54)"
              /> */}

              {/* <YAxis
                yAxisId="resister_fan_pwm"
                orientation="right"
                domain={[0, 100]}
                width={30}
                stroke="#ff0000"
              /> */}
              <Tooltip />
              <Legend />
              <Line
                yAxisId="voltage"
                type="monotone"
                dataKey="Volt"
                stroke="#ff0000"
                dot={false} // Disable dots
              />
              <Line
                yAxisId="current"
                type="monotone"
                dataKey="Current"
                stroke="green"
                dot={false} // Disable dots
              />
              {/* <Line
                yAxisId="soc"
                type="monotone"
                dataKey="SoC"
                stroke="blue"
                dot={false} // Disable dots
              /> */}
              {/* <Line
                yAxisId="temperature"
                type="monotone"
                dataKey="Charge_Temp"
                stroke="green"
                dot={false} // Disable dots
              /> */}
              {/* <Line
                yAxisId="resister_temp"
                type="monotone"
                dataKey="Discharge_Temp"
                stroke="orange"
                dot={false} // Disable dots
              /> */}
              <Line
                yAxisId="duty_cycle"
                type="monotone"
                dataKey="Charge_PWM"
                stroke="#ff33cc"
                dot={false} // Disable dots
              />

              {/* <Line
                yAxisId="fan_pwm"
                type="monotone"
                dataKey="Charge_Fan_PWM"
                stroke="rgb(255 111 54)"
                dot={false} // Disable dots
              /> */}

              {/* <Line
                yAxisId="resister_fan_pwm"
                type="monotone"
                dataKey="Discharge_Fan_PWM"
                stroke="#ff0000"
                dot={false} // Disable dots
              /> */}
            </LineChart>
          </ResponsiveContainer>
        </DataContainer>
      )}
    </Container>
  );
};

export default DataPage;

const Container = styled.div`
  padding: 20px;
`;

const Title = styled.div`
  font-size: 30px;
  font-weight: 600;
  margin-bottom: 15px;
`;

const Form = styled.form`
  display: flex;
  margin-bottom: 20px;
  label {
    margin-right: 20px;
  }
  input,
  select {
    margin-left: 10px;
  }
`;

const DataContainer = styled.div`
  background: #ecebff;
  border-radius: 15px;
  padding: 10px 5px 5px 5px;
`;
