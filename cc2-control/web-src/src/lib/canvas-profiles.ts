import { store } from './store'
import { request, post } from './api'

export type CanvasMaterialProfile = 'pla' | 'petg'

export type CanvasProfilePrefs = {
  profile: CanvasMaterialProfile
  pla: { min: number; max: number }
  petg: { min: number; max: number }
  singleMaterialRefillOff: boolean
}

const defaults = (): CanvasProfilePrefs => ({
  profile: 'pla',
  pla: { min: 190, max: 220 },
  petg: { min: 230, max: 250 },
  singleMaterialRefillOff: true,
})

export const canvasProfiles = store(defaults())

export function readCanvasProfilesFromApi(p: Record<string, unknown>) {
  const d = defaults()
  if (p.canvas_material_profile === 'petg') d.profile = 'petg'
  const num = (k: string, fallback: number) => {
    const v = Number(p[k])
    return Number.isFinite(v) ? Math.round(v) : fallback
  }
  d.pla.min = num('canvas_profile_pla_min', d.pla.min)
  d.pla.max = num('canvas_profile_pla_max', d.pla.max)
  d.petg.min = num('canvas_profile_petg_min', d.petg.min)
  d.petg.max = num('canvas_profile_petg_max', d.petg.max)
  d.singleMaterialRefillOff = p.canvas_single_material_refill_off !== false
  if (d.pla.min >= d.pla.max || d.pla.min < 120 || d.pla.max > 320) d.pla = defaults().pla
  if (d.petg.min >= d.petg.max || d.petg.min < 120 || d.petg.max > 320) d.petg = defaults().petg
  canvasProfiles.set(d)
}

export async function saveCanvasProfiles(partial: Partial<CanvasProfilePrefs>) {
  const cur = { ...canvasProfiles.get(), ...partial }
  if (cur.pla.min >= cur.pla.max || cur.petg.min >= cur.petg.max) throw Error('Invalid temperature range')
  await request('/api/preferences', {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json', 'X-CC2-Request': '1' },
    body: JSON.stringify({
      canvas_material_profile: cur.profile,
      canvas_profile_pla_min: cur.pla.min,
      canvas_profile_pla_max: cur.pla.max,
      canvas_profile_petg_min: cur.petg.min,
      canvas_profile_petg_max: cur.petg.max,
      canvas_single_material_refill_off: cur.singleMaterialRefillOff,
    }),
  })
  canvasProfiles.set(cur)
}

export async function maybeDisableAutoRefillForSingleMaterial() {
  if (!canvasProfiles.get().singleMaterialRefillOff) return
  try {
    await post('/api/canvas/auto-refill', 'off')
  } catch {
    /* best-effort */
  }
}
