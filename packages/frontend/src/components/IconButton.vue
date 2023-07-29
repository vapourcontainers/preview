<template>
  <a-tooltip placement="topLeft" :mouse-enter-delay="1">
    <template #title v-if="props.label && props.compact && !props.disabled">{{ props.label }}</template>
    <a-button :type="primary ? 'primary' : 'default'" :loading="props.loading" :disabled="props.disabled">
      <template #icon v-if="props.icon">
        <component :is="props.icon" />
      </template>
      <template v-if="!props.compact">
        {{ props.label }}
      </template>
    </a-button>
  </a-tooltip>
</template>

<script lang="ts" setup>
import type { FunctionalComponent } from 'vue';

const props = defineProps<{
  label?: string,
  icon?: FunctionalComponent,
  compact?: boolean;
  primary?: boolean;
  loading?: boolean;
  disabled?: boolean;
}>();
</script>
