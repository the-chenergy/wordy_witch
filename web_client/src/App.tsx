import React from 'react';
import logo from './logo.svg';
import './App.css';

import { default as createBotModule } from './library/wordyWitchBot.mjs';
import { MainModule as BotModule } from './library/wordyWitchBot';

const App = () => {
  createBotModule().then((Bot: BotModule) => {
    console.log('Bot', Bot);
    let words = new Bot.vector_string();
    for (let w of ['teary', 'timer', 'tears', 'timed']) {
      words.push_back(w);
    }
    let numTargets = 2;
    Bot.load_bank(words, numTargets);
    console.log('Done, apparently');
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
