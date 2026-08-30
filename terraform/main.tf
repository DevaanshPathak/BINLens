resource "docker_image" "binlens" {
  name         = "binlens:${var.image_tag}"
  build {
    context    = "${path.module}/.."
    dockerfile = "${path.module}/../Dockerfile"
  }
}

resource "docker_container" "binlens" {
  name  = var.container_name
  image = docker_image.binlens.image_id
  rm    = true

  dynamic "ports" {
    for_each = var.host_port > 0 ? [1] : []
    content {
      internal = 80
      external = var.host_port
    }
  }
}