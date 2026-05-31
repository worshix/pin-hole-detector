from django.contrib import admin

from .models import Capture, SystemState


@admin.register(Capture)
class CaptureAdmin(admin.ModelAdmin):
    list_display = ('id', 'device_id', 'status', 'pinholes_found', 'created_at')
    list_filter = ('status', 'device_id', 'created_at')
    search_fields = ('device_id', 'notes')


@admin.register(SystemState)
class SystemStateAdmin(admin.ModelAdmin):
    list_display = ('key', 'value', 'updated_at')
