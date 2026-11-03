import { useEffect, useState } from 'preact/hooks'
import { ls } from '@/lib/store'
import { t } from '@/lib/i18n'
import { Button } from '@/components/ui/button'

type BeforeInstallPromptEvent = Event & { prompt: () => Promise<void> }

const DISMISS_KEY = 'cc2-pwa-install-dismissed'

export const PwaInstallBanner = () => {
  const [deferred, setDeferred] = useState<BeforeInstallPromptEvent | null>(null)
  const [hidden, setHidden] = useState(() => ls.get(DISMISS_KEY) === '1')
  const [installed, setInstalled] = useState(false)

  useEffect(() => {
    if (
      window.matchMedia('(display-mode: standalone)').matches ||
      (navigator as Navigator & { standalone?: boolean }).standalone
    ) {
      setInstalled(true)
      return
    }
    const onPrompt = (e: Event) => {
      e.preventDefault()
      setDeferred(e as BeforeInstallPromptEvent)
    }
    window.addEventListener('beforeinstallprompt', onPrompt)
    return () => window.removeEventListener('beforeinstallprompt', onPrompt)
  }, [])

  if (hidden || installed) return null

  const dismiss = () => {
    ls.set(DISMISS_KEY, '1')
    setHidden(true)
  }

  return (
    <section
      class="mb-3.5 rounded-lg border border-cyan/30 bg-panel px-3 py-2.5 text-sm"
      aria-label={t('pwa.install_title')}
    >
      <p class="font-semibold">{t('pwa.install_title')}</p>
      <p class="mt-1 text-xs text-muted">{t('pwa.install_hint')}</p>
      <p class="mt-1 text-xs text-muted">{t('pwa.install_hint_other')}</p>
      <div class="mt-2 flex flex-wrap gap-2">
        <Button variant="primary" onClick={dismiss}>
          {t('pwa.got_it')}
        </Button>
        {deferred ? (
          <Button
            variant="ghost"
            onClick={() => {
              void deferred.prompt()
              setDeferred(null)
              dismiss()
            }}
          >
            {t('pwa.install_button_chrome')}
          </Button>
        ) : null}
        <Button variant="ghost" onClick={dismiss}>
          {t('pwa.dismiss')}
        </Button>
      </div>
    </section>
  )
}

export function registerServiceWorker() {
  if (!import.meta.env.PROD || !('serviceWorker' in navigator)) return
  window.addEventListener('load', () => {
    void navigator.serviceWorker.register('/sw.js', { scope: '/' }).catch(() => {})
  })
}
