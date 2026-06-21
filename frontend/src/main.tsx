import React from 'react';
import ReactDOM from 'react-dom/client';
import { BrowserRouter } from 'react-router-dom';
import { ConfigProvider, theme, App as AntApp } from 'antd';
import App from './App';
import './styles/global.css';

const darkTheme = {
  algorithm: theme.darkAlgorithm,
  token: {
    colorPrimary: '#00E5FF',
    colorSuccess: '#00FF88',
    colorError: '#FF4757',
    colorWarning: '#FFA502',
    colorBgBase: '#0D1117',
    colorBgContainer: 'rgba(255,255,255,0.03)',
    colorBorder: 'rgba(255,255,255,0.06)',
    colorText: '#E6E6E6',
    colorTextSecondary: '#8B949E',
    borderRadius: 8,
  },
};

ReactDOM.createRoot(document.getElementById('root')!).render(
  <React.StrictMode>
    <BrowserRouter>
      <ConfigProvider theme={darkTheme}>
        <AntApp>
          <App />
        </AntApp>
      </ConfigProvider>
    </BrowserRouter>
  </React.StrictMode>
);
