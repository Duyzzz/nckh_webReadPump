const ctx1 = document.getElementById('chart1').getContext('2d');
const ctx2 = document.getElementById('chart2').getContext('2d');
const ctx3 = document.getElementById('chart3').getContext('2d');

const myChart1 = new Chart(ctx1, {
    type: 'line',
    data: { labels: [], datasets: [{ label: 'Tần số (Hz)', data: [], borderColor: 'rgba(75, 192, 192, 1)', borderWidth: 1 }] },
    options: { scales: { y: { beginAtZero: true } } }
});

const myChart2 = new Chart(ctx2, {
    type: 'line',
    data: { labels: [], datasets: [{ label: 'Tần số (Hz)', data: [], borderColor: 'rgba(75, 192, 192, 1)', borderWidth: 1 }] },
    options: { scales: { y: { beginAtZero: true } } }
});

const myChart3 = new Chart(ctx3, {
    type: 'line',
    data: { labels: [], datasets: [{ label: 'Tần số (Hz)', data: [], borderColor: 'rgba(75, 192, 192, 1)', borderWidth: 1 }] },
    options: { scales: { y: { beginAtZero: true } } }
});

let intervalId = null; // Khởi tạo biến intervalId

function updateChart(chart, data) {
    chart.data.labels.push(new Date().toLocaleTimeString());
    chart.data.datasets[0].data.push(data);
    chart.update();
}

function startUpdating(pumpNumber) {
    clearInterval(intervalId); // Dừng cập nhật trước đó
    // Ẩn tất cả các biểu đồ
    document.getElementById('chart1').style.display = 'none';
    document.getElementById('chart2').style.display = 'none';
    document.getElementById('chart3').style.display = 'none';

    // Hiển thị biểu đồ tương ứng
    if (pumpNumber === 1) {
        document.getElementById('chart1').style.display = 'block';
        intervalId = setInterval(() => {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    updateChart(myChart1, data["value"])
                    console.log(data["value"])
                })
                .catch(error => console.error("Error fetching data:", error));
        }, 200);
    } else if (pumpNumber === 2) {
        document.getElementById('chart2').style.display = 'block';
        intervalId = setInterval(() => {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    updateChart(myChart2, data["pump2"])
                    console.log(data["pump2"])
                })
                .catch(error => console.error("Error fetching data:", error));
        }, 200);
    } else if (pumpNumber === 3) {
        document.getElementById('chart3').style.display = 'block';
        intervalId = setInterval(() => {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    updateChart(myChart3, data["pump3"])
                    console.log(data["pump3"])
                })
                .catch(error => console.error("Error fetching data:", error));
        }, 200);
    }
}

document.getElementById('pump1').addEventListener('click', () => startUpdating(1));
document.getElementById('pump2').addEventListener('click', () => startUpdating(2));
document.getElementById('pump3').addEventListener('click', () => startUpdating(3));

