import React from 'react';
import logo from './logo.svg';
import './App.css';

import createModule from './library/wordyWitchBot.mjs';

const App = () => {
  createModule().then((x: any) => {
    console.log(x);
    x.loadBank(['TEARY', 'TIMER', 'TEARS', 'TIMED'], 2);
    console.log('Done! Apparently');
  });

  return (
    <div className='App'>
      <header className='App-header'>
        <img src={logo} className='App-logo' alt='logo' />
        <p>
          Edit <code>src/App.tsx</code> and save to reload.
        </p>
        <a
          className='App-link'
          href='https://reactjs.org'
          target='_blank'
          rel='noopener noreferrer'
        >
          Learn React
        </a>
      </header>
    </div>
  );
};

export default App;
