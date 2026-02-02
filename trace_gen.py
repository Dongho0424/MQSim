import random


class MQSimTraceGenerator:
    def __init__(
        self,
        channel=8,
        chip=4,
        die=2,
        plane=2,
        block=2048,
        page=256,
        page_cap_bytes=8192,
        op_ratio=0.07,
    ):
        self.sector_size = 512
        self.sectors_per_page = page_cap_bytes // self.sector_size

        self.total_physical_sectors = channel * chip * die * plane * block * page * self.sectors_per_page
        self.max_lba = int(self.total_physical_sectors * (1 - op_ratio))  # In host side

        print(f"[Info] Total Physical Sectors: {self.total_physical_sectors}")
        print(f"[Info] SSD Capacity: {self.total_physical_sectors * self.sector_size / (1024 ** 3)} GB")
        print(f"[Info] Max User LBA: {self.max_lba}")

        # Request Settings
        self.req_size_sectors = 8  # 4KB / 512B (fixed)
        self.device_no = 0  # dummy value (fixed)
        self.read_type = 1
        self.write_type = 0

    def generate_enns_trace(self, filename, num_requests, start_vec_idx=0, inter_arrival_time_ns=1000):
        """
        ENNS: Bulk Sequential Read
        - 0start_vec_idx: 읽기를 시작할 벡터의 인덱스 (0번째 벡터 = LBA 0)
        - inter_arrival_time_ns: 요청 간 시간 간격 (순차 읽기이므로 짧게 설정)
        """
        current_time = 0

        with open(filename, "w") as f:
            for i in range(num_requests):
                # 4KB 단위로 순차 증가
                # Vector Index i -> LBA = i * 8
                target_lba = (start_vec_idx + i) * self.req_size_sectors

                # 범위 체크 (Wrap around or Stop)
                if target_lba + self.req_size_sectors > self.max_lba:
                    print(f"[Warning] LBA exceeded Max LBA at request {i}. Wrapping around.")
                    target_lba = target_lba % self.max_lba

                # Format: Time Device LBA Size Type
                line = f"{current_time} {self.device_no} {target_lba} {self.req_size_sectors} {self.read_type}\n"
                f.write(line)

                current_time += inter_arrival_time_ns

        print(f"[Success] ENNS trace generated: {filename} ({num_requests} requests)")

    def generate_hnsw_trace(self, filename, num_requests, total_vectors_in_db, inter_arrival_time_ns=5000):
        """
        HNSW: Graph Based / Random Read
        - total_vectors_in_db: DB에 저장된 총 벡터 개수 (이 범위 내에서 랜덤 선택)
        - inter_arrival_time_ns: 요청 간 시간 간격 (탐색 연산 고려하여 순차보다 길게 설정 가능)
        """
        current_time = 0

        # DB가 차지하는 마지막 LBA 계산
        db_end_lba = total_vectors_in_db * self.req_size_sectors
        if db_end_lba > self.max_lba:
            print("[Warning] DB size is larger than SSD User Capacity!")
            db_end_lba = self.max_lba

        with open(filename, "w") as f:
            for i in range(num_requests):
                # DB 범위 내에서 랜덤한 벡터 인덱스 선택
                rand_vec_idx = random.randint(0, total_vectors_in_db - 1)
                target_lba = rand_vec_idx * self.req_size_sectors

                # Format: Time Device LBA Size Type
                line = f"{current_time} {self.device_no} {target_lba} {self.req_size_sectors} {self.read_type}\n"
                f.write(line)

                current_time += inter_arrival_time_ns

        print(f"[Success] HNSW trace generated: {filename} ({num_requests} requests)")


# ==========================================
# 실행부 (Configuration)
# ==========================================

# 1. Generator 초기화 (상수 설정)
generator = MQSimTraceGenerator(
    channel=8,
    chip=4,
    die=2,
    plane=2,
    block=2048,
    page=256,
    page_cap_bytes=8192,
    op_ratio=0.07,
)

# 2. 파라미터 설정
NUM_REQUESTS = 1_000_000  # 생성할 요청 개수
# DB에 저장된 벡터 수
# 4GB: 1M
# 40GB: 10M
TOTAL_VECTORS = 10_000_000

# 3. Trace 생성
trace_dir = "traces/"

generator.generate_enns_trace(
    filename=f"{trace_dir}enns_seq_1.trace",
    num_requests=NUM_REQUESTS,
    start_vec_idx=0,
    inter_arrival_time_ns=1,
)


generator.generate_hnsw_trace(
    filename=f"{trace_dir}hnsw_rand_1.trace",
    num_requests=NUM_REQUESTS,
    total_vectors_in_db=TOTAL_VECTORS,
    inter_arrival_time_ns=1,
)
