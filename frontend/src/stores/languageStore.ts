import { create } from 'zustand';
import translations from '../utils/translations';

type Lang = 'en' | 'ru';

interface LanguageState {
  lang: Lang;
  setLang: (l: Lang) => void;
  t: (key: string) => string;
}

export const useLanguageStore = create<LanguageState>((set, get) => ({
  lang: (localStorage.getItem('obs_lang') as Lang) || 'ru',
  setLang: (l) => {
    localStorage.setItem('obs_lang', l);
    set({ lang: l });
  },
  t: (key: string) => {
    const lang = get().lang;
    return translations[lang]?.[key] || translations['en']?.[key] || key;
  },
}));
