import {
  Card,
  CssBaseline,
  CssVarsProvider,
  Grid,
  Sheet,
  Typography,
  extendTheme,
  useColorScheme,
} from '@mui/joy';
import { useEffect, useState } from 'react';
import { Stack } from '@mui/system';
import Bot from './library/bot';

const myTheme = extendTheme({
  fontFamily: { body: 'Montserrat', display: 'Montserrat' },
});

const App = () => {
  useEffect(() => {
    console.log('Bot', Bot);
    let words = new Bot.StringVector();
    for (let w of ['TEARY', 'TIMER', 'TEARS', 'TIMED']) {
      words.push_back(w);
    }
    Bot.loadBank(words, 2);
    console.log('Done, apparently');
  }, []);

  const LetterCard = ({
    letter,
    verdict,
    size = 'medium',
    unit = 1,
  }: {
    letter: string;
    verdict: string;
    size?: 'medium' | 'small';
    unit?: number;
  }) => {
    const colorScheme = useColorScheme();
    const colorMode =
      colorScheme.mode === 'system' ? colorScheme.systemMode : colorScheme.mode;

    return (
      <Card
        variant='plain'
        color={
          verdict === '#' ? 'success' : verdict === '^' ? 'warning' : 'neutral'
        }
        sx={{
          width: (size === 'small' ? 36 : 56) * unit + 'px',
          height: (size === 'small' ? 48 : 56) + 'px',
          padding: 0,
          boxShadow:
            verdict === ' '
              ? 'inset 0px 0px 0px 1px var(--joy-palette-neutral-400)'
              : undefined,
          backgroundColor: {
            '?': 'neutral.plainActiveBg',
            '-': 'neutral.plainDisabledColor',
            '^': 'warning.400',
            '#': 'success.400',
          }[verdict],
          display: 'flex',
          justifyContent: 'center',
          alignItems: 'center',
          userSelect: 'none',
          cursor: 'pointer',
          pointerEvents:
            size === 'medium' && verdict === ' ' ? 'none' : undefined,
          ':hover': {
            filter:
              colorMode === 'dark'
                ? verdict === ' '
                  ? 'brightness(1.33)'
                  : 'brightness(1.17)'
                : 'brightness(0.92)',
          },
          ':active': {
            filter:
              colorMode === 'dark'
                ? verdict === ' '
                  ? 'brightness(2)'
                  : 'brightness(1.33)'
                : 'brightness(0.83)',
          },
        }}
      >
        <Typography
          fontSize={
            size === 'small' ? 'min(16px, 100vw / 48)' : 'min(30px, 100vw / 24)'
          }
          fontWeight={size === 'small' ? 600 : 700}
          textColor={
            verdict === ' ' || verdict === '?' ? 'text.primary' : 'common.white'
          }
        >
          {letter}
        </Typography>
      </Card>
    );
  };

  const WordGrid = ({
    words,
    verdicts,
  }: {
    words: string[];
    verdicts: string[];
  }) => {
    const WordCardGroup = ({
      word,
      verdict,
    }: {
      word: string;
      verdict: string;
    }) => (
      <Stack direction='row' spacing='8px' maxWidth='100%'>
        {word.split('').map((letter, i) => (
          <LetterCard
            key={i + letter}
            letter={letter}
            verdict={verdict.charAt(i)}
          />
        ))}
      </Stack>
    );

    return (
      <Stack spacing={1} maxWidth='100%'>
        {words.map((word, i) => (
          <WordCardGroup key={i + word} word={word} verdict={verdicts[i]} />
        ))}
      </Stack>
    );
  };

  const OnScreenKeyboard = ({
    verdictByLetter,
  }: {
    verdictByLetter: { [key: string]: string };
  }) => {
    const keys = [
      ['Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'],
      ['A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'],
      ['Z', 'X', 'C', 'V', 'B', 'N', 'M', 'delete'],
    ];

    const KeyCard = ({ letter }: { letter: string }) => {
      if (letter === 'delete') {
        return <LetterCard size='small' unit={1.5} letter='⌫' verdict=' ' />;
      } else {
        return (
          <LetterCard
            size='small'
            letter={letter}
            verdict={verdictByLetter[letter] || ' '}
          />
        );
      }
    };

    return (
      <Stack spacing='8px' alignItems='center' minWidth={0} maxWidth='100%'>
        {keys.map((row, i) => (
          <Stack
            key={i}
            direction='row'
            spacing='6px'
            minWidth={0}
            maxWidth='100%'
            sx={{
              transform: i === 1 ? 'translateX(-4px)' : undefined,
            }}
          >
            {row.map((letter, i) => (
              <KeyCard key={i + letter} letter={letter} />
            ))}
          </Stack>
        ))}
      </Stack>
    );
  };

  const GameBoard = ({
    words,
    verdicts,
  }: {
    words: string[];
    verdicts: string[];
  }) => {
    const [verdictByLetter, setVerdictByLetter] = useState<{
      [key: string]: string;
    }>({});

    useEffect(() => {
      let newVerdictByLetter: { [key: string]: string } = {};
      const markTile = (tile: string) => {
        for (let i = 0; i < verdicts.length; i++) {
          for (let j = 0; j < words[i].length; j++) {
            if (verdicts[i].charAt(j) === tile) {
              newVerdictByLetter[words[i].charAt(j)] = tile;
            }
          }
        }
        setVerdictByLetter(newVerdictByLetter);
      };
      markTile('#');
      markTile('^');
      markTile('-');
    }, [words, verdicts]);

    return (
      <Stack spacing='32px' maxWidth='100%' alignItems='center'>
        <WordGrid
          words={new Array(6)
            .fill(null)
            .map((_, i) => (i < words.length ? words[i] : '     '))}
          verdicts={new Array(6)
            .fill(null)
            .map((_, i) => (i < verdicts.length ? verdicts[i] : '     '))}
        />
        <OnScreenKeyboard verdictByLetter={verdictByLetter} />
      </Stack>
    );
  };

  return (
    <>
      <CssBaseline />
      <CssVarsProvider theme={myTheme} defaultMode='system'>
        <Sheet sx={{ minHeight: '100vh', paddingTop: '24px' }}>
          <Grid container margin='auto' padding='16px'>
            <Grid
              height='100%'
              xs={6}
              display='flex'
              alignItems='center'
              justifyContent='flex-end'
            >
              <GameBoard
                words={['STARE', 'CLOUD', 'PINKY']}
                verdicts={['#-^--', '-^--#', '?????']}
              />
            </Grid>
          </Grid>
        </Sheet>
      </CssVarsProvider>
    </>
  );
};

export default App;
