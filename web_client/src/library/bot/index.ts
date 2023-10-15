import createModule from './bot.mjs';
import { MainModule } from './bot';

export default (await createModule()) as MainModule;
