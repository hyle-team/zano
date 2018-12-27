import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { OpenWalletComponent } from './open-wallet.component';

describe('OpenWalletComponent', () => {
  let component: OpenWalletComponent;
  let fixture: ComponentFixture<OpenWalletComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ OpenWalletComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(OpenWalletComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
