from django.db import models


class Capture(models.Model):
    STATUS_CHOICES = [
        ('pending', 'Pending Analysis'),
        ('clean', 'No Defect'),
        ('defect', 'Defect Found'),
    ]

    image = models.ImageField(upload_to='captures/%Y/%m/%d/')
    device_id = models.CharField(max_length=80, blank=True)
    status = models.CharField(max_length=20, choices=STATUS_CHOICES, default='pending')
    pinholes_found = models.PositiveIntegerField(default=0)
    notes = models.CharField(max_length=255, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)

    class Meta:
        ordering = ['-created_at']

    def __str__(self):
        return f'Capture {self.id} - {self.created_at:%Y-%m-%d %H:%M:%S}'


class SystemState(models.Model):
    key = models.CharField(max_length=50, unique=True)
    value = models.CharField(max_length=100)
    updated_at = models.DateTimeField(auto_now=True)

    def __str__(self):
        return f'{self.key}: {self.value}'
