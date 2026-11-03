import { useEffect, useState } from 'preact/hooks'
import { Card, CardHead } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input, Select, Switch } from '@/components/ui/field'
import { errText, notify } from '@/lib/api'
import { t, type Key } from '@/lib/i18n'
import {
  DEFAULT_TIMELAPSE,
  loadTimelapseSettings,
  saveTimelapseSettings,
  TIMELAPSE_USB_PICTURE,
  TIMELAPSE_USB_VIDEO,
  validateTimelapseForm,
  type TimelapseSettings,
} from '@/lib/timelapse-settings'
import {
  TIMELAPSE_QUALITY_PRESETS,
  timelapseLowSpace,
  timelapsePresetFromValues,
  type TimelapseQualityPreset,
} from '@/lib/timelapse-presets'
import { request } from '@/lib/api'
import { openSettings } from '@/lib/state'

const PRESETS = ['ultrafast', 'superfast', 'veryfast', 'faster', 'fast', 'medium', 'slow', 'slower', 'veryslow']
const PROFILES = ['baseline', 'main', 'high']

const Field = ({ label, htmlFor, children }: { label: Key; htmlFor?: string; children: preact.ComponentChildren }) => (
  <div class="my-2 grid gap-1 cc2-md:grid-cols-[170px_minmax(180px,480px)] cc2-md:items-center">
    {htmlFor ? (
      <label htmlFor={htmlFor} class="text-sm font-semibold">
        {t(label)}
      </label>
    ) : (
      <span class="text-sm font-semibold">{t(label)}</span>
    )}
    {children}
  </div>
)

export const TimelapseSettingsPage = () => {
  const [form, setForm] = useState<TimelapseSettings>(DEFAULT_TIMELAPSE)
  const [busy, setBusy] = useState(false)
  const [usbMounted, setUsbMounted] = useState(false)

  useEffect(() => {
    void loadTimelapseSettings().then(data => {
      if (data) setForm(data)
    })
    void request('/api/gcode-files')
      .then((data: { usb?: { available?: boolean } }) => setUsbMounted(data.usb?.available === true))
      .catch(() => setUsbMounted(false))
  }, [])

  const patch = (partial: Partial<TimelapseSettings>) => setForm(f => ({ ...f, ...partial }))

  const save = async () => {
    if (!validateTimelapseForm(form)) {
      notify(t('timelapse.path_invalid'), 'error')
      return
    }
    setBusy(true)
    try {
      await saveTimelapseSettings({
        recording_mode: form.recording_mode,
        width: form.width,
        height: form.height,
        camera_fps: form.camera_fps,
        output_fps: form.output_fps,
        bit_rate: form.bit_rate,
        crf: form.crf,
        preset: form.preset,
        tune: form.tune,
        profile: form.profile,
        level: form.level,
        pic_max_size: form.pic_max_size,
        keep_source_jpegs: form.keep_source_jpegs,
        picture_dir: form.picture_dir,
        video_dir: form.video_dir,
        temp_link: form.temp_link,
      })
      notify(t('timelapse.saved'))
    } catch (e) {
      notify(`${t('timelapse.save_failed')} ${errText(e)}`, 'error')
    } finally {
      setBusy(false)
    }
  }

  const quality = timelapsePresetFromValues(form)
  const lowSpace = timelapseLowSpace(form.picture_space, form.video_space)

  return (
    <Card>
      <CardHead icon="camera" title="timelapse.title" sub="timelapse.subtitle" />
      <p class="mb-3.5 rounded-md border border-amber/40 bg-panel px-3 py-2 text-xs text-amber" role="status">
        {t('timelapse.firmware_banner')}
      </p>
      <Field label="timelapse.recording_mode" htmlFor="tl-mode">
        <Select
          id="tl-mode"
          value={form.recording_mode}
          onChange={e => patch({ recording_mode: e.currentTarget.value })}
        >
          <option value="layer_timelapse">{t('timelapse.recording_layer')}</option>
          <option value="realtime_timelapse">{t('timelapse.recording_realtime')}</option>
        </Select>
      </Field>
      {form.recording_mode === 'realtime_timelapse' && (
        <p class="mb-2 text-xs text-muted cc2-md:col-start-2">{t('timelapse.recording_realtime_help')}</p>
      )}
      <Field label="timelapse.quality_preset" htmlFor="tl-qual">
        <Select
          id="tl-qual"
          value={quality}
          onChange={e => {
            const v = e.currentTarget.value as TimelapseQualityPreset
            if (v === 'custom') return
            patch(TIMELAPSE_QUALITY_PRESETS[v])
          }}
        >
          <option value="low">{t('timelapse.quality_low')}</option>
          <option value="medium">{t('timelapse.quality_medium')}</option>
          <option value="high">{t('timelapse.quality_high')}</option>
          <option value="custom">{t('timelapse.quality_custom')}</option>
        </Select>
      </Field>
      <h3 class="mb-2 mt-3.5 text-base font-semibold">{t('timelapse.resolution')}</h3>
      <Field label="timelapse.width" htmlFor="tl-w">
        <Input
          id="tl-w"
          type="number"
          min={320}
          max={1920}
          step={2}
          value={String(form.width)}
          onInput={e => patch({ width: Number(e.currentTarget.value) })}
        />
      </Field>
      <Field label="timelapse.height" htmlFor="tl-h">
        <Input
          id="tl-h"
          type="number"
          min={240}
          max={1080}
          step={2}
          value={String(form.height)}
          onInput={e => patch({ height: Number(e.currentTarget.value) })}
        />
      </Field>
      <Field label="timelapse.camera_fps" htmlFor="tl-cfps">
        <Input
          id="tl-cfps"
          type="number"
          min={5}
          max={30}
          value={String(form.camera_fps)}
          onInput={e => patch({ camera_fps: Number(e.currentTarget.value) })}
        />
      </Field>
      <Field label="timelapse.output_fps" htmlFor="tl-ofps">
        <Input
          id="tl-ofps"
          type="number"
          min={10}
          max={60}
          value={String(form.output_fps)}
          onInput={e => patch({ output_fps: Number(e.currentTarget.value) })}
        />
      </Field>
      {quality === 'custom' ? (
        <>
          <h3 class="mb-2 mt-3.5 text-base font-semibold">{t('timelapse.encoder')}</h3>
          <Field label="timelapse.bit_rate" htmlFor="tl-br">
            <Input
              id="tl-br"
              type="number"
              min={500000}
              max={20000000}
              step={100000}
              value={String(form.bit_rate)}
              onInput={e => patch({ bit_rate: Number(e.currentTarget.value) })}
            />
          </Field>
          <Field label="timelapse.crf" htmlFor="tl-crf">
            <Input
              id="tl-crf"
              type="number"
              min={0}
              max={51}
              value={String(form.crf)}
              onInput={e => patch({ crf: Number(e.currentTarget.value) })}
            />
          </Field>
          <Field label="timelapse.preset" htmlFor="tl-preset">
            <Select id="tl-preset" value={form.preset} onChange={e => patch({ preset: e.currentTarget.value })}>
              {PRESETS.map(p => (
                <option key={p} value={p}>
                  {p}
                </option>
              ))}
            </Select>
          </Field>
          <Field label="timelapse.profile" htmlFor="tl-prof">
            <Select id="tl-prof" value={form.profile} onChange={e => patch({ profile: e.currentTarget.value })}>
              {PROFILES.map(p => (
                <option key={p} value={p}>
                  {p}
                </option>
              ))}
            </Select>
          </Field>
          <Field label="timelapse.level" htmlFor="tl-lv">
            <Input id="tl-lv" value={form.level} onInput={e => patch({ level: e.currentTarget.value })} />
          </Field>
          <Field label="timelapse.pic_max_size" htmlFor="tl-pic">
            <Input
              id="tl-pic"
              type="number"
              min={102400}
              max={1048576}
              step={1024}
              value={String(form.pic_max_size)}
              onInput={e => patch({ pic_max_size: Number(e.currentTarget.value) })}
            />
          </Field>
          <div class="my-2 flex flex-wrap items-center gap-2 cc2-md:pl-[170px]">
            <Switch
              on={form.keep_source_jpegs}
              label={t('timelapse.keep_source_jpegs')}
              onClick={() => patch({ keep_source_jpegs: !form.keep_source_jpegs })}
            />
            <span class="text-sm">{t('timelapse.keep_source_jpegs')}</span>
          </div>
        </>
      ) : null}
      <h3 class="mb-2 mt-3.5 text-base font-semibold">{t('timelapse.storage')}</h3>
      <p class="mb-2 text-xs text-muted">{t('timelapse.usb_hint')}</p>
      <p class="mb-2 text-xs text-amber">{t('timelapse.usb_risk')}</p>
      {usbMounted ? (
        <div class="mb-2">
          <Button
            variant="ghost"
            class="h-auto px-2 py-1 text-xs"
            disabled={busy}
            onClick={() => patch({ picture_dir: TIMELAPSE_USB_PICTURE, video_dir: TIMELAPSE_USB_VIDEO })}
          >
            {t('timelapse.use_usb_paths')}
          </Button>
        </div>
      ) : null}
      <Field label="timelapse.picture_dir" htmlFor="tl-pdir">
        <Input
          id="tl-pdir"
          value={form.picture_dir}
          spellcheck={false}
          onInput={e => patch({ picture_dir: e.currentTarget.value })}
        />
      </Field>
      <Field label="timelapse.video_dir" htmlFor="tl-vdir">
        <Input
          id="tl-vdir"
          value={form.video_dir}
          spellcheck={false}
          onInput={e => patch({ video_dir: e.currentTarget.value })}
        />
      </Field>
      <Field label="timelapse.temp_link" htmlFor="tl-tmp">
        <Input
          id="tl-tmp"
          value={form.temp_link}
          spellcheck={false}
          onInput={e => patch({ temp_link: e.currentTarget.value })}
        />
      </Field>
      {lowSpace ? (
        <p class="mb-2 text-xs text-amber" role="status">
          {t('timelapse.low_space_warning')}
        </p>
      ) : null}
      <div class="mt-3.5 flex flex-wrap gap-2">
        <Button disabled={busy} onClick={() => void save()}>
          {t('timelapse.save')}
        </Button>
        <Button variant="ghost" disabled={busy} onClick={() => setForm(DEFAULT_TIMELAPSE)}>
          {t('timelapse.reset')}
        </Button>
      </div>
    </Card>
  )
}

export const TimelapseSettingsLink = () => (
  <button
    type="button"
    class="text-left text-xs text-cyan underline-offset-2 hover:underline"
    onClick={() => openSettings('timelapse')}
  >
    {t('print.timelapse_settings_hint')}
  </button>
)
