from django.contrib import messages
from django.core.files.base import ContentFile
from django.db.models import Count, Sum
from django.http import HttpResponseBadRequest, JsonResponse
from django.shortcuts import redirect, render
from django.utils import timezone
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_POST

from .models import Capture, SystemState
from .mqtt import publish_control


def get_machine_state():
    state, _ = SystemState.objects.get_or_create(key='machine_state', defaults={'value': 'idle'})
    return state


def dashboard(request):
    captures = Capture.objects.all()
    latest_capture = captures.first()
    recent_captures = captures[:8]
    summary = captures.aggregate(total=Count('id'), pinholes=Sum('pinholes_found'))
    defect_count = captures.filter(status='defect').count()
    machine_state = get_machine_state()

    context = {
        'latest_capture': latest_capture,
        'recent_captures': recent_captures,
        'total_images': summary['total'] or 0,
        'total_pinholes': summary['pinholes'] or 0,
        'defect_count': defect_count,
        'machine_state': machine_state.value,
        'last_updated': machine_state.updated_at,
    }
    return render(request, 'detector/dashboard.html', context)


def history(request):
    captures = Capture.objects.all()
    return render(request, 'detector/history.html', {'captures': captures})


@require_POST
def control_machine(request, command):
    if command not in {'start', 'stop'}:
        return HttpResponseBadRequest('Unsupported command')

    state = get_machine_state()
    state.value = 'running' if command == 'start' else 'stopped'
    state.save(update_fields=['value', 'updated_at'])

    try:
        publish_control(command)
        messages.success(request, f'{command.title()} command sent to ESP32 devices.')
    except Exception as exc:
        messages.error(request, f'MQTT command failed: {exc}')

    return redirect('dashboard')


@csrf_exempt
def receive_image(request):
    if request.method != 'POST':
        return JsonResponse({'detail': 'POST an image/jpeg body to this endpoint.'}, status=405)

    content_type = request.headers.get('Content-Type', '')
    uploaded_file = request.FILES.get('image')
    device_id = request.headers.get('X-Device-ID', request.POST.get('device_id', 'esp32-cam'))

    if uploaded_file:
        capture = Capture.objects.create(image=uploaded_file, device_id=device_id)
    elif 'image/jpeg' in content_type and request.body:
        filename = f'esp32cam_{timezone.now():%Y%m%d_%H%M%S_%f}.jpg'
        capture = Capture(device_id=device_id)
        capture.image.save(filename, ContentFile(request.body), save=True)
    else:
        return JsonResponse({'detail': 'No JPEG image was received.'}, status=400)

    SystemState.objects.update_or_create(key='last_camera_upload', defaults={'value': str(capture.id)})
    return JsonResponse({'ok': True, 'id': capture.id, 'image_url': capture.image.url}, status=201)
