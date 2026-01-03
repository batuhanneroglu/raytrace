'use client';

import { useEffect, useRef, useState } from 'react';

interface Vec3 {
  x: number;
  y: number;
  z: number;
}

interface Sphere {
  center: Vec3;
  radius: number;
  color: Vec3;
}

interface Light {
  position: Vec3;
  color: Vec3;
  intensity: number;
}

export default function Home() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const [lightPosition, setLightPosition] = useState<Vec3>({ x: 3, y: 0, z: -5 });
  const [isDragging, setIsDragging] = useState(false);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const width = canvas.width;
    const height = canvas.height;

    // Scene setup
    const sphere: Sphere = {
      center: { x: -2, y: 0, z: -5 },
      radius: 1.5,
      color: { x: 1, y: 1, z: 1 }
    };

    const light: Light = {
      position: lightPosition,
      color: { x: 1, y: 0.95, z: 0.7 },
      intensity: 2.5
    };

    // Vector operations
    const vec3 = {
      subtract: (a: Vec3, b: Vec3): Vec3 => ({ x: a.x - b.x, y: a.y - b.y, z: a.z - b.z }),
      add: (a: Vec3, b: Vec3): Vec3 => ({ x: a.x + b.x, y: a.y + b.y, z: a.z + b.z }),
      scale: (v: Vec3, s: number): Vec3 => ({ x: v.x * s, y: v.y * s, z: v.z * s }),
      dot: (a: Vec3, b: Vec3): number => a.x * b.x + a.y * b.y + a.z * b.z,
      length: (v: Vec3): number => Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z),
      normalize: (v: Vec3): Vec3 => {
        const len = vec3.length(v);
        return len > 0 ? { x: v.x / len, y: v.y / len, z: v.z / len } : { x: 0, y: 0, z: 0 };
      }
    };

    // Ray-sphere intersection - returns distance or -1
    const intersectSphere = (origin: Vec3, direction: Vec3, sphere: Sphere): number => {
      const oc = vec3.subtract(origin, sphere.center);
      const a = vec3.dot(direction, direction);
      const b = 2.0 * vec3.dot(oc, direction);
      const c = vec3.dot(oc, oc) - sphere.radius * sphere.radius;
      const discriminant = b * b - 4 * a * c;

      if (discriminant < 0) return -1;
      return (-b - Math.sqrt(discriminant)) / (2.0 * a);
    };

    // Check if light source is inside sphere
    const lightInsideSphere = vec3.length(vec3.subtract(light.position, sphere.center)) < sphere.radius;

    // Raytracing - simple white sphere
    const trace = (origin: Vec3, direction: Vec3): Vec3 => {
      const t = intersectSphere(origin, direction, sphere);

      if (t > 0) {
        return { x: 1, y: 1, z: 1 };
      }

      return { x: 0, y: 0, z: 0 };
    };

    // Render sphere
    const imageData = ctx.createImageData(width, height);
    const data = imageData.data;

    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const u = (x / width) * 2 - 1;
        const v = (y / height) * 2 - 1;

        const origin = { x: 0, y: 0, z: 0 };
        const direction = vec3.normalize({ x: u * (width / height), y: -v, z: -1 });

        const color = trace(origin, direction);

        const idx = (y * width + x) * 4;
        data[idx] = Math.min(255, color.x * 255);
        data[idx + 1] = Math.min(255, color.y * 255);
        data[idx + 2] = Math.min(255, color.z * 255);
        data[idx + 3] = 255;
      }
    }

    ctx.putImageData(imageData, 0, 0);

    // Only draw rays if light is NOT inside sphere
    if (!lightInsideSphere) {
      // Draw light rays with gradient
      const numRays = 360;
      for (let i = 0; i < numRays; i++) {
        const angle = (i / numRays) * Math.PI * 2;
        const rayDir = vec3.normalize({
          x: Math.cos(angle),
          y: Math.sin(angle),
          z: 0
        });

        const screenLightPos = {
          x: ((light.position.x / Math.abs(light.position.z)) * (height / width) + 1) * width / 2,
          y: ((-light.position.y / Math.abs(light.position.z)) + 1) * height / 2
        };

        // Check if ray hits sphere
        const t = intersectSphere(light.position, rayDir, sphere);
        
        // Calculate end point based on whether ray hits sphere
        let endPoint: Vec3;
        let maxDistance = 15;
        
        if (t > 0.01 && t < maxDistance) {
          // Ray hits sphere - stop at intersection point
          endPoint = vec3.add(light.position, vec3.scale(rayDir, t));
        } else {
          // Ray doesn't hit sphere - extend to maximum distance
          endPoint = vec3.add(light.position, vec3.scale(rayDir, maxDistance));
        }

        const screenEnd = {
          x: ((endPoint.x / Math.abs(endPoint.z)) * (height / width) + 1) * width / 2,
          y: ((-endPoint.y / Math.abs(endPoint.z)) + 1) * height / 2
        };

        // Create gradient for each ray - MUCH brighter at source
        const gradient = ctx.createLinearGradient(
          screenLightPos.x, screenLightPos.y,
          screenEnd.x, screenEnd.y
        );
        
        // Much brighter near the source, matching the glow
        gradient.addColorStop(0, 'rgba(255, 255, 255, 1)');
        gradient.addColorStop(0.1, 'rgba(255, 245, 200, 0.95)');
        gradient.addColorStop(0.3, 'rgba(255, 240, 180, 0.7)');
        gradient.addColorStop(0.6, 'rgba(255, 240, 180, 0.3)');
        gradient.addColorStop(1, 'rgba(255, 240, 180, 0.05)');

        ctx.beginPath();
        ctx.moveTo(screenLightPos.x, screenLightPos.y);
        ctx.lineTo(screenEnd.x, screenEnd.y);
        
        ctx.strokeStyle = gradient;
        ctx.lineWidth = 1.2;
        ctx.stroke();
      }
    }

    // Draw light source glow
    const screenLightPos = {
      x: ((light.position.x / Math.abs(light.position.z)) * (height / width) + 1) * width / 2,
      y: ((-light.position.y / Math.abs(light.position.z)) + 1) * height / 2
    };

    const gradient = ctx.createRadialGradient(
      screenLightPos.x, screenLightPos.y, 0,
      screenLightPos.x, screenLightPos.y, 60
    );
    gradient.addColorStop(0, 'rgba(255, 255, 255, 1)');
    gradient.addColorStop(0.3, 'rgba(255, 240, 150, 0.9)');
    gradient.addColorStop(0.6, 'rgba(255, 220, 100, 0.4)');
    gradient.addColorStop(1, 'rgba(255, 200, 80, 0)');

    ctx.fillStyle = gradient;
    ctx.beginPath();
    ctx.arc(screenLightPos.x, screenLightPos.y, 60, 0, Math.PI * 2);
    ctx.fill();
  }, [lightPosition]);

  const handleMouseDown = (e: React.MouseEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const screenLightPos = {
      x: ((lightPosition.x / Math.abs(lightPosition.z)) * (canvas.height / canvas.width) + 1) * canvas.width / 2,
      y: ((-lightPosition.y / Math.abs(lightPosition.z)) + 1) * canvas.height / 2
    };

    const distance = Math.sqrt(
      Math.pow(mouseX - screenLightPos.x, 2) + 
      Math.pow(mouseY - screenLightPos.y, 2)
    );

    if (distance < 60) {
      setIsDragging(true);
    }
  };

  const handleMouseMove = (e: React.MouseEvent<HTMLCanvasElement>) => {
    if (!isDragging) return;

    const canvas = canvasRef.current;
    if (!canvas) return;

    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const u = (mouseX / canvas.width) * 2 - 1;
    const v = (mouseY / canvas.height) * 2 - 1;

    const newX = u * Math.abs(lightPosition.z) / (canvas.height / canvas.width);
    const newY = -v * Math.abs(lightPosition.z);

    setLightPosition({ 
      x: newX, 
      y: newY, 
      z: lightPosition.z 
    });
  };

  const handleMouseUp = () => {
    setIsDragging(false);
  };

  return (
    <div className="flex min-h-screen items-center justify-center bg-black">
      <div className="flex flex-col items-center gap-4">
        <h1 className="text-2xl font-bold text-white">Raytracing</h1>
        <canvas
          ref={canvasRef}
          width={1200}
          height={800}
          className="border border-gray-700 cursor-pointer"
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={handleMouseUp}
        />
      </div>
    </div>
  );
}
