import { errText, notify, request } from '@/lib/api'
import { t } from '@/lib/i18n'

export type TimelapseSettings = {
  loaded: boolean
  firmware_support: boolean
  path: string
  recording_mode: string
  width: number
  height: number
  camera_fps: number
  output_fps: number
  bit_rate: number
  crf: number
  preset: string
  tune: string
  profile: string
  level: string
  pic_max_size: number
  keep_source_jpegs: boolean
  picture_dir: string
  video_dir: string
  temp_link: string
}

export const DEFAULT_TIMELAPSE: TimelapseSettings = {
  loaded: true,
  firmware_support: false,
  path: '/opt/usr/cfg/timelapse.cfg',
  recording_mode: 'layer_timelapse',
  width: 640,
  height: 360,
  camera_fps: 20,
  output_fps: 30,
  bit_rate: 4_000_000,
  crf: 20,
  preset: 'medium',
  tune: 'stillimage',
  profile: 'main',
  level: '3.1',
  pic_max_size: 307_200,
  keep_source_jpegs: false,
  picture_dir: '/opt/usr/picture/',
  video_dir: '/opt/usr/video/',
  temp_link: '/tmp/pic_link',
}

export async function fetchTimelapseSettings(): Promise<TimelapseSettings> {
  const data = await request('/api/timelapse/settings')
  return { ...DEFAULT_TIMELAPSE, ...data, keep_source_jpegs: data.keep_source_jpegs === true }
}

export async function saveTimelapseSettings(body: Partial<TimelapseSettings>) {
  return request('/api/timelapse/settings', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'X-CC2-Request': '1' },
    body: JSON.stringify(body),
  })
}

export async function loadTimelapseSettings(): Promise<TimelapseSettings | null> {
  try {
    return await fetchTimelapseSettings()
  } catch (e) {
    notify(`${t('timelapse.load_failed')} ${errText(e)}`, 'error')
    return null
  }
}
