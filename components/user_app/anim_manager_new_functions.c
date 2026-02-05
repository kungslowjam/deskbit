
uint8_t anim_manager_get_count(void) { return anim_count; }

const char *anim_manager_get_name_by_index(uint8_t index) {
  if (index >= anim_count) {
    return NULL;
  }
  return anim_registry[index].name;
}
