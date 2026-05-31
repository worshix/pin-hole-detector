# Generated manually for the pinhole detector application.

from django.db import migrations, models


class Migration(migrations.Migration):

    initial = True

    dependencies = []

    operations = [
        migrations.CreateModel(
            name='Capture',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('image', models.ImageField(upload_to='captures/%Y/%m/%d/')),
                ('device_id', models.CharField(blank=True, max_length=80)),
                ('status', models.CharField(choices=[('pending', 'Pending Analysis'), ('clean', 'No Defect'), ('defect', 'Defect Found')], default='pending', max_length=20)),
                ('pinholes_found', models.PositiveIntegerField(default=0)),
                ('notes', models.CharField(blank=True, max_length=255)),
                ('created_at', models.DateTimeField(auto_now_add=True)),
            ],
            options={
                'ordering': ['-created_at'],
            },
        ),
        migrations.CreateModel(
            name='SystemState',
            fields=[
                ('id', models.BigAutoField(auto_created=True, primary_key=True, serialize=False, verbose_name='ID')),
                ('key', models.CharField(max_length=50, unique=True)),
                ('value', models.CharField(max_length=100)),
                ('updated_at', models.DateTimeField(auto_now=True)),
            ],
        ),
    ]
