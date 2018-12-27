import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { StakingSwitchComponent } from './staking-switch.component';

describe('StakingSwitchComponent', () => {
  let component: StakingSwitchComponent;
  let fixture: ComponentFixture<StakingSwitchComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ StakingSwitchComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(StakingSwitchComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
